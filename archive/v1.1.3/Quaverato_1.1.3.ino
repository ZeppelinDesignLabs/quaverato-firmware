#define _TASK_MICRO_RES


#include <TaskScheduler.h> //https://github.com/arkhipenko/TaskScheduler

#include <ramp256.h>
#include <saw256.h>
#include <sine256.h> //This table has been modified 
#include <sq256.h>
#include <tri256.h>

#include <EEPROM.h>

const byte version = 1;
const byte release = 1;
const byte patch = 3;

const byte led_pin_TempoLed = 3;
const byte led_pin_Low = 10;
const byte led_pin_High = 9;
const byte led_pin_Bypass = 4;

const byte switch_pin_TapTempo = 2;
const byte switch_pin_Bypass = 1;
const byte switch_pin_Mode = 5;
const byte switch_pin_Phase = 6;

const byte relay_pin_Switch = 8;
const byte relay_pin_Lock = 7;


//Analog Numbers
const byte pot_pin_Rate = 1; 
const byte pot_pin_Shape = 4;
const byte pot_pin_Depth = 0;
const byte pot_pin_DutyCycle = 3;
const byte pot_pin_TimeDivision = 2;
const byte pot_pin_HarmonicMix = 5;

const int minTapRate = 784;   //actual minimum is 784/4 = 196
const int maxTapRate = 15625; //actual maximum is 15625*2 = 31250 microseconds

unsigned long stepRate = 196;
unsigned long firstHalfStepRate = 196;
unsigned long secondHalfStepRate = 196;
float dutyCycle = 0.5;

volatile bool relayON = false;

volatile unsigned long debounceTimer = 0;
volatile unsigned long buttonTimer = 0;
volatile unsigned long tempoTapped = 0;


const byte *waveTable[] = {&sine256[0], &saw256[0], &ramp256[0], &tri256[0], &sq256[0]};
byte currentWaveTable = 0;
byte waveFormStep = 0;

byte depth = 16;
double tapDivisor = 1;
int floorOne = 16;
int floorTwo = 16;

bool synchronize = true;
bool dutyCycleFlagged = false;

bool callibrationMode = false;
byte mixTarget = 255;

byte zeroCutoff = 0;


Scheduler ts;

void scopefix(){} //required to let the compiler know that following constructors are in the right scope to point to their respective functions
Task oscillator(784, TASK_FOREVER, &stepWaveform, &ts);
Task checkControls(4000, TASK_FOREVER, &functionSwitch, &ts);
Task relayCleanup(TASK_IMMEDIATE, TASK_FOREVER, &resetRelay, &ts);
Task dutyCycleCleanup(TASK_IMMEDIATE, TASK_FOREVER, &updateDutyCycle, &ts);




byte adjustLight(int lightIntensity, int mix){
	lightIntensity = (lightIntensity * mix) / 32;
	lightIntensity = lightIntensity + ((mixTarget * (32 - mix)) / 32); 
	lightIntensity = (lightIntensity * depth) / 32;
	lightIntensity = lightIntensity + ((255 * (32 - depth)) / 32); 
	return (byte)lightIntensity;
}


void light(byte lightIntensity){
	lightIntensity = lightIntensity < zeroCutoff ? 0 : lightIntensity;
	byte lightOne = adjustLight(lightIntensity, floorOne); 
	if (!synchronize){
		lightIntensity = 255 - lightIntensity;
	}
	byte lightTwo = adjustLight(lightIntensity, floorTwo);
  
    if (floorOne >= floorTwo){
		analogWrite(led_pin_TempoLed,lightOne);
		lightTwo = callibrationMode ? 0 : lightTwo;
    }
    else{
      analogWrite(led_pin_TempoLed,lightTwo);
      lightOne = callibrationMode ? 0 : lightOne;
    }
	analogWrite(led_pin_High,lightOne);
	analogWrite(led_pin_Low, lightTwo);
}


void stepWaveform(){  
	light(pgm_read_byte_near(waveTable[currentWaveTable] + waveFormStep));
	waveFormStep++;
	if (waveFormStep == 0){
    if (dutyCycleFlagged){
       setTempo(stepRate);
       dutyCycleFlagged = false;
    }
		oscillator.setInterval(firstHalfStepRate);
	}
	if (waveFormStep == 128){
		oscillator.setInterval(secondHalfStepRate);
	}
}

  
void tap(){  
	if  (micros() - debounceTimer > 10000){
		if (micros()- buttonTimer < 3000000){
			tempoTapped = micros() - buttonTimer;
		}
		buttonTimer = micros();
		debounceTimer = micros();
		attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), debounceTap, RISING);
	}
}

void debounceTap(){
	if  (micros() - debounceTimer > 10000){
		debounceTimer = micros();
		attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), tap, FALLING);
	}
}

void tapped(){
	if (tempoTapped > 0){
		stepRate = constrain(tempoTapped/256, minTapRate, maxTapRate);  
		setTempo(stepRate);
		tempoTapped = 0;
	}
}
unsigned long applyTapDivision(unsigned long rate){
	return rate /= tapDivisor;
}

void setTempo(unsigned long tempo){
	splitDutyCycle(dutyCycle, applyTapDivision(stepRate));
	oscillator.setInterval(firstHalfStepRate);
	oscillator.enable();
}

void functionSwitch(){
	static int function = 0;
	
	function++;
if (function > 8) {function = 0;}

	switch  (function){
		case 0:
			ratePot();
			break;
		case 1:
			waveformPot();
			break;
		case 2:
			depthPot();
			break;
		case 3:
			divisionPot();
			break;
		case 4: 
			harmonicMixPot();
			break;
		case 5:
			dutyCyclePot();
			break;
		case 6:
			phaseSwitch();
			break;
		case 7:
			modeSwitch();
			break;
		case 8:
			tapped();
			break;
	}
}


 void phaseSwitch(){
  synchronize = !digitalRead(switch_pin_Phase);
 }
  
void modeSwitch(){
  static bool momentMode = false;
  static bool switchMode = false;
  static bool pushed = false;
  noInterrupts();
  if (digitalRead(switch_pin_Mode) == LOW){
    if (!switchMode){     
       if (momentMode){
        relayON = true;
        flipRelay();
        momentMode = false;    
      }
      switchMode = true;         
    }
     if (digitalRead(switch_pin_Bypass) == LOW && !pushed){
        pushed = true;
        flipRelay();        
      }
      else if (digitalRead(switch_pin_Bypass) == HIGH && pushed){
        pushed = false;     
      }
  }
  else {
      if (digitalRead(switch_pin_Bypass) == relayON){
        flipRelay();        
      }
      momentMode = true;
      switchMode = false;
  }
  interrupts();
}
  
 void ratePot(){
	static int oldPotRead0 = -11;
	static int oldStepRate = -1;
	if (analogRead(pot_pin_Rate) > oldPotRead0 + 10 || analogRead(pot_pin_Rate) < oldPotRead0 - 10){  
		stepRate = map(analogRead(pot_pin_Rate), 0, 1023, 125, 16);
		stepRate *= stepRate;
		if (stepRate != oldStepRate){
		dutyCycleFlagged = true;
    dutyCycleCleanup.enableDelayed(2000000);
			oldStepRate = stepRate;
		}
		oldPotRead0 = analogRead(pot_pin_Rate);
	}
 }
 
 void waveformPot(){
	currentWaveTable = map(analogRead(pot_pin_Shape), -100, 900,0,4);
	currentWaveTable = constrain(currentWaveTable, 0, 4);
 }
 
 void depthPot(){
	depth = map(analogRead(pot_pin_Depth),0, 1023,1,102);
	depth = log10(depth) * 16;
	depth = min(depth, 32);
 }
 
 void divisionPot(){
	static int timeDivisionOld = 0;
	int timeDivision = map(analogRead(pot_pin_TimeDivision), -150, 900, 1, 6);
	timeDivision = constrain(timeDivision, 1, 6);
	if (timeDivision != timeDivisionOld){
		switch (timeDivision){
			case 1:
				tapDivisor = 0.5;
				break;
			case 2:
				tapDivisor = 1.0;
				break;
			case 3:
				tapDivisor = 1.5;
				break;
			case 4:
				tapDivisor = 2.0;
				break;
			case 5:
				tapDivisor = 3.0;
				break;
			case 6:
				tapDivisor = 4.0;
				break;
		}
		setTempo(stepRate);
		timeDivisionOld = timeDivision;
	}	
 }
 
 void harmonicMixPot(){
	int harmonicMix = analogRead(pot_pin_HarmonicMix);
	if (harmonicMix <= 450){
		floorOne =  map(harmonicMix, 450, 0, 100, 0);
		floorOne = log10(floorOne) * 16;
		floorOne = min(floorOne, 32);
		floorTwo = 32;
	}
	else if (harmonicMix >= 550){
		floorOne = 32;
		floorTwo = map(harmonicMix, 550, 1023, 100, 0); 
		floorTwo = log10(floorTwo) * 16;
		floorTwo = min(floorTwo, 32);  
	}
	else{
		floorOne = 32;
		floorTwo = 32;
	}
	zeroCutoff = abs(floorOne - floorTwo);
	if (callibrationMode){
		if (floorOne != 32){floorOne = 0;}
		if (floorTwo != 32){floorTwo = 0;}
	}
 }
 
void dutyCyclePot(){
	static int oldPotRead = -10;
	if (analogRead(pot_pin_DutyCycle) > oldPotRead + 10 || analogRead(pot_pin_DutyCycle) < oldPotRead - 10){
		dutyCycle = double(analogRead(pot_pin_DutyCycle)) / 1023;
		dutyCycle = constrain(dutyCycle, 0.0625, 0.9375);
    dutyCycleFlagged = true;
    dutyCycleCleanup.enableDelayed(2000000);
		//setTempo(stepRate);
		oldPotRead = analogRead(pot_pin_DutyCycle);
	}
}

void updateDutyCycle(){
if (dutyCycleFlagged)
       setTempo(stepRate);
       dutyCycleFlagged = false;  
       dutyCycleCleanup.disable();       
}

void splitDutyCycle(double duty, unsigned long rate){
  if (!synchronize){
    rate *= 2;
  }
  firstHalfStepRate = 2 * rate * duty;
  secondHalfStepRate = 2 * rate * (1 - duty);
}

void flipRelay(){
  static unsigned long buttonTimer = 0;
  noInterrupts();
  if  (micros() - buttonTimer > 20000){
		relayON = !relayON;
		digitalWrite(led_pin_Bypass, relayON);
		digitalWrite(relay_pin_Lock, LOW);
		digitalWrite(relay_pin_Switch, LOW);
		digitalWrite(relay_pin_Lock, !relayON);
		digitalWrite(relay_pin_Switch, relayON);
		relayCleanup.enableDelayed(200000);
	}
  buttonTimer = micros();
  interrupts();
}

void resetRelay(){
	digitalWrite(relay_pin_Lock, LOW);
	digitalWrite(relay_pin_Switch, LOW);
	relayCleanup.disable();
}
//###################################################
//           Startup Options
//###################################################
bool scanForCallibrationMode(){
	int counter = 0;
	while (!digitalRead(switch_pin_TapTempo) && !digitalRead(switch_pin_Bypass)){
		delay(100);
		counter++;
		if (counter > 30){
			return true;
		}
	}
	return false;
}

void enterCallibrationMode(){
	bool lightOn = false;
	callibrationMode = true;
	mixTarget = 0;
	for (int i = 0; i <15; i++){
		lightOn = !lightOn;
		digitalWrite(led_pin_Bypass, lightOn);
		digitalWrite(led_pin_TempoLed, lightOn);
		digitalWrite(led_pin_High, lightOn);
		digitalWrite(led_pin_Low, lightOn);
		delay(100);
	}
}

bool scanForModeChange(){
	int counter = 0;
	while (!digitalRead(switch_pin_Bypass)){
		delay(100);
		counter++;
		if (counter >= 30){
			return true;
		}
	}
	return false;
}

void changeMode(){
	bool lightON = false;
	bool mode = EEPROM.read(1);
	EEPROM.write(1,!mode);
	for (int i = 0; i < (1 + mode)*4; i++){
		lightON = !lightON;
		digitalWrite(led_pin_Bypass, lightON);
		delay(100);
	}  
}

bool scanForVersionMode(){
  int counter = 0;
  while (!digitalRead(switch_pin_TapTempo)){
    delay(100);
    counter++;
    if (counter >= 30){
      return true;
    }
  }
  return false;
}

void runVersionMode(){
  while (true){    
	versionBlink(version);	
    versionBlink(release);	
    versionBlink(patch);
	
    digitalWrite(led_pin_TempoLed, LOW);
    delay(4000);
  }
}

void versionBlink(int numberOfBlinks){
	for (int i = numberOfBlinks + 1; i > 0; i--){
     digitalWrite(led_pin_TempoLed, LOW);
     delay(150);
     digitalWrite(led_pin_TempoLed, HIGH);
     delay(150);
    }
	digitalWrite(led_pin_TempoLed, LOW);
    delay(800);
}

//###################################################  
//             Initialization        
//###################################################

void setup() {

	TCCR1B = (TCCR1B & 0b11111000) | 0x01; //Sets the PWM frequency divisor to 1

	pinMode(led_pin_TempoLed, OUTPUT);
	pinMode(led_pin_Bypass, OUTPUT);
	pinMode(relay_pin_Lock, OUTPUT);
	pinMode(relay_pin_Switch, OUTPUT);
	pinMode(switch_pin_TapTempo, INPUT_PULLUP);
	pinMode(switch_pin_Mode,INPUT_PULLUP);
	pinMode(switch_pin_Phase,INPUT_PULLUP);
	pinMode(switch_pin_Bypass, INPUT_PULLUP);
  
	digitalWrite(switch_pin_Bypass, HIGH);

	if (scanForCallibrationMode()){
		enterCallibrationMode();
	}
	if (scanForVersionMode()){
		runVersionMode();
    }
	if (scanForModeChange()){
		changeMode();
	}
  delay(200);
	relayON = EEPROM.read(1);
	flipRelay();
  
	attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), tap, FALLING);
  
	oscillator.enable();
	checkControls.enable();
}

void loop() {
ts.execute();
}
