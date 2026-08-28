
#define _TASK_MICRO_RES


#include <TaskScheduler.h> //https://github.com/arkhipenko/TaskScheduler

#include <ramp256.h>
#include <saw256.h>
#include <sine256.h> //This table has been modified 
#include <sq256.h>
#include <tri256.h>
#include <MIDI.h>
#include <EEPROMex.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const byte version = 2;
const byte release = 4;
const byte patch = 2;

const byte led_pin_TempoLed = 3;
const byte led_pin_Low = 10;
const byte led_pin_High = 9;
const byte led_pin_Bypass = 4;

const byte midi_pin_Input = 0;

const byte switch_pin_TapTempo = 2;
const byte switch_pin_Bypass = 1;
const byte switch_pin_Mode = 5;
const byte switch_pin_Phase = 6;

const byte relay_pin_Switch = 8;
const byte relay_pin_Lock = 7;
const byte relay_pin_Isolator = 11;


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

int timeDivision = 2;

bool synchronize = true;


bool callibrationMode = false;
byte mixTarget = 255;

byte zeroCutoff = 0;

bool tableShift = false;

bool setMidiChannel = false;
bool midiOmni = false;
int midiChannel = 0;
bool followMidiClock = false;
int clockCount = 0;

  bool momentMode = false;
  bool switchMode = false; 
  bool presetModeFlag = false; 

int expressionSelect = 20;

int presetOffset = 0;
const int presetRoot = 10;

struct Preset{
  byte depth;
  double multiplier;
  byte waveShape;
  unsigned long rate;
  float spacing;
  int harmonicMixHigh;
  int harmonicMixLow;
  bool phase;
}currentPreset;



Scheduler ts; 

void scopefix(){} //required to let the compiler know that following constructors are in the right scope to point to their respective functions
Task oscillator(784, TASK_FOREVER, &stepWaveform, &ts);
Task checkControls(4000, TASK_FOREVER, &functionSwitch, &ts);
Task relayPartTwo(TASK_IMMEDIATE, TASK_FOREVER, &flipRelayPartTwo, &ts);
Task relayCleanup(TASK_IMMEDIATE, TASK_FOREVER, &resetRelay, &ts);
Task relayDisableIsolator(TASK_IMMEDIATE, TASK_FOREVER, &disableIsolator, &ts);
//Task dutyCycleCleanup(TASK_IMMEDIATE, TASK_FOREVER, &updateDutyCycle, &ts);
Task updateMidi(160, TASK_FOREVER, &handleMIDI, &ts);
Task storePreset(20000, TASK_FOREVER, &presetMode, &ts);
Task midiMomentaryMode(TASK_IMMEDIATE, TASK_FOREVER, &midiMomentRelay, &ts);

//###################################################
//          MIDI implementation
//###################################################

//Midi hook
void handleMIDI(){
  MIDI.read();
}

void handleControlChange(byte channel, byte number, byte value){
  if (setMidiChannel){
    midiChannel = channel;
    EEPROM.write(2, channel);
    EEPROM.write(3, false);
    MIDI.setInputChannel(midiChannel);
    setMidiChannel = false;
  }
  switch (number){
    case 20:
      depth = log10(value) * 16;
      depth = min(depth, 32);
      break;
    case 21:
    if (value <=21){
    tapDivisor = 0.5;
    }
    else if (value <= 43){
      tapDivisor = 1.0;
    }
    else if (value <= 65){
       tapDivisor = 1.5;
    }
    else if (value <= 87){
      tapDivisor = 2.0;
    }
    else if (value <= 109){
      tapDivisor = 3.0;
    }
    else {
      tapDivisor = 4.0;
    }
     setTempo(stepRate);
    break;
    case 22:
    if (value <=25){
    currentWaveTable = 0;
    }
    else if (value <= 50){
      currentWaveTable = 1;
    }
    else if (value <= 75){
       currentWaveTable = 2;
    }
    else if (value <= 100){
      currentWaveTable = 3;
    }
    else {
      currentWaveTable = 4;
    }
    break;
    case 24:
    stepRate = 143 - value; //minimum = 16
    stepRate *= stepRate;
    setTempo(stepRate);
    break;
    case 25:
    dutyCycle = (float)value / 127;
    dutyCycle = constrain(dutyCycle, 0.0625, 0.9375);
     setTempo(stepRate);
    break;
    case 26:
      if (value > 63){
    floorOne = 32;
    floorTwo = map(value, 64, 127, 100, 0); 
    floorTwo = log10(floorTwo) * 16;
    floorTwo = min(floorTwo, 32);  
      }
      else {
    floorOne =  map(value, 0, 63, 0, 100);
    floorOne = log10(floorOne) * 16;
    floorOne = min(floorOne, 32);
    floorTwo = 32;
      }
    break;
    case 27:
    if (value > 63){
      synchronize = true;
    }
    else synchronize = false;
    break;
    case 28:
    if (value > 63){
      momentMode = true;
      switchMode = false;
    }
    else {
      momentMode = false;
      switchMode = true;
    }
    break;
    case 29:
    flipRelay();
    break;
    case 30:
    handleControlChange(channel, expressionSelect, value);
    break;
    case 35:
    if (value > 63){
      tableShift = true;
    }
    else tableShift = false;
    break;
    case 51: 
    if (value > 63){
      followMidiClock = true;
    }
    else followMidiClock = false;
    break;
    case 93:
    tap();
    break;
    case 124:
    midiChannel = EEPROM.read(2);
    EEPROM.write(3, false);
    MIDI.setInputChannel(midiChannel);   
    break;
    case 125:
    midiChannel = MIDI_CHANNEL_OMNI;
    EEPROM.write(3, true);
    MIDI.setInputChannel(midiChannel);
    break;
    default:
    break;   
  }
  
}
void handleProgramChange(byte channel, byte number){
  const int minPreset = 0;
  const int maxPreset = 5;
  if (setMidiChannel){
    midiChannel = channel;
    EEPROM.write(2, channel);
    MIDI.setInputChannel(midiChannel);
    setMidiChannel = false;
  }  
  if (number >= minPreset && number <= maxPreset){
    readPreset(number);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity){
  if (velocity == 0){
    handleNoteOff(channel, note, velocity);
  }
  else if (!relayON){
    midiMomentaryMode.enableDelayed(20000);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity){
    if (relayON){
    midiMomentaryMode.enableDelayed(20000);
  }
}

void handleClock(){
  
  clockCount++;
   if (followMidiClock && clockCount >= 24){
    if (micros()- buttonTimer < 3000000){
      tempoTapped = micros() - buttonTimer;
    }
    buttonTimer = micros();
  clockCount = 0;
   }
}
void handleStart(){
  if (followMidiClock){
    clockCount = 0;
    buttonTimer = micros();
    if (tableShift){
      waveFormStep = 128;
      oscillator.setInterval(secondHalfStepRate);
      oscillator.enable();
    }
    else{
  waveFormStep = 0;
  oscillator.setInterval(firstHalfStepRate);
  oscillator.enable();
  }
  }
}
void handleStop(){
   if (followMidiClock){
  oscillator.disable();
   }
}
void handleContinue(){
   if (followMidiClock){
  oscillator.enable();
   }
}
//###################################################
//          Output driver
//###################################################

void stepWaveform(){  
	light(pgm_read_byte_near(waveTable[currentWaveTable] + waveFormStep));
	waveFormStep++;
	if (waveFormStep < 128){
    if (!followMidiClock || waveFormStep == 0){   
		oscillator.setInterval(firstHalfStepRate);
    }
	}
	if (waveFormStep >= 128){
  if (!followMidiClock || waveFormStep == 128){  
		oscillator.setInterval(secondHalfStepRate);
  }
  }
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


byte adjustLight(int lightIntensity, int mix){
	lightIntensity = (lightIntensity * mix) / 32;
	lightIntensity = lightIntensity + ((mixTarget * (32 - mix)) / 32); 
	lightIntensity = (lightIntensity * depth) / 32;
	lightIntensity = lightIntensity + ((255 * (32 - depth)) / 32); 
	return (byte)lightIntensity;
}



//###################################################
//          Control driver
//###################################################

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
      bypassSwitch();
			break;
		case 8:
			tapped();
			break;
	}
}


//###################################################
//          Controls
//###################################################
 void phaseSwitch(){
  static bool oldReading = !digitalRead(switch_pin_Phase);
  bool reading = digitalRead(switch_pin_Phase);
  if (reading != oldReading){
  synchronize = !reading;
  oldReading = reading;
  }
 }
 
 void modeSwitch(){

  static bool oldReading;
  bool reading = digitalRead(switch_pin_Mode);
  if (reading != oldReading && !presetModeFlag){
  noInterrupts();
  if (reading == LOW){
    if (!switchMode){     
       if (momentMode){
        relayON = true;
        flipRelay();
        momentMode = false;    
      }
      switchMode = true;         
    }
  }
  else {      
      momentMode = true;
      switchMode = false;
  }
  interrupts();
  oldReading = reading;
  }
}

void bypassSwitch(){
  static bool pushed = false;
  bool reading = digitalRead(switch_pin_Mode);
  if (switchMode && !presetModeFlag){
         if (digitalRead(switch_pin_Bypass) == LOW && !pushed){
        pushed = true;
    storePreset.enableDelayed(5000000);
        flipRelay();        
      }
      else if (digitalRead(switch_pin_Bypass) == HIGH && pushed){
        pushed = false;
        if (!presetModeFlag){ 
    storePreset.disable();
        }
      }
  }
  else if (momentMode){
    if (digitalRead(switch_pin_Bypass) == relayON){
        flipRelay();        
      }
  }
}

 void ratePot(){
	static int oldReading = -10;
	static int oldStepRate = -1;
 int reading = analogRead(pot_pin_Rate);
  if (reading >= oldReading + 10 || reading <= oldReading -10){ 
		stepRate = map(reading, 0, 1023, 125, 16);
		stepRate *= stepRate;
		if (stepRate != oldStepRate){
       setTempo(stepRate);
			oldStepRate = stepRate;
		}
    expressionSelect = 24;
		oldReading = reading;
	}
 } 


 void waveformPot(){
  static int oldReading = -10;
    int reading = analogRead(pot_pin_Shape);
  if (reading >= oldReading + 10 || reading <= oldReading -10){
	currentWaveTable = map(reading, -100, 900,0,4);
	currentWaveTable = constrain(currentWaveTable, 0, 4);
   expressionSelect = 22;
   oldReading = reading;
  }
 }
 

 void depthPot(){
  static int oldReading = -10;
 
  int reading = analogRead(pot_pin_Depth);
  if (reading >= oldReading + 10 || reading <= oldReading -10){
	depth  = map(reading,0, 1023,1,102);
	depth = log10(depth) * 16;
	depth = min(depth, 32);
  expressionSelect = 20;
  oldReading = reading;
  }
 }

 void divisionPot(){
	static int oldReading = -10;
  static int timeDivisionOld = -10;
 
  int reading = analogRead(pot_pin_TimeDivision);
  if (reading >= oldReading + 10 || reading <= oldReading -10){
	timeDivision = map(reading, -150, 900, 1, 6);
	timeDivision = constrain(timeDivision, 1, 6);
	if (timeDivision != timeDivisionOld && !presetModeFlag){
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
		oldReading = reading;
   timeDivisionOld = timeDivision;
	}	
      expressionSelect = 21;
      oldReading = reading;
  }
 }

  void harmonicMixPot(){
	static int oldReading = -10;
  
  int reading = analogRead(pot_pin_HarmonicMix);
  if (reading >= oldReading + 10 || reading <= oldReading -10){
    
	int harmonicMix = reading;
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
  expressionSelect = 26;
	oldReading = reading;
	}
 }
 
void dutyCyclePot(){
	static int oldReading = -10;
  
	int reading = analogRead(pot_pin_DutyCycle);
  if (reading >= oldReading + 10 || reading <= oldReading -10){
		dutyCycle = (float)reading / 1023;
		dutyCycle = constrain(dutyCycle, 0.0625, 0.9375);
       setTempo(stepRate);
    expressionSelect = 25;
		oldReading = reading;
	}
}

//###################################################
//          Tap Handling
//###################################################

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

//###################################################
//          Tempo and Shape Handling
//###################################################
unsigned long applyTapDivision(unsigned long rate){
	return rate /= tapDivisor;
}

void setTempo(unsigned long tempo){
	splitDutyCycle(dutyCycle, applyTapDivision(stepRate));
	oscillator.enable();
}

void splitDutyCycle(double duty, unsigned long rate){
  if (!synchronize){
    rate *= 2;
  }
  firstHalfStepRate = 2 * rate * duty;
  secondHalfStepRate = 2 * rate * (1 - duty);
}

//###################################################
//           Bypass
//###################################################


void flipRelay(){
  static unsigned long buttonTimer = 0;
  noInterrupts();
  
  if  (micros() - buttonTimer > 25000){
    digitalWrite(relay_pin_Isolator, LOW);
		relayPartTwo.enableDelayed(25000);
	}
  buttonTimer = micros();
  interrupts();
}

void flipRelayPartTwo(){
  relayON = !relayON;
    digitalWrite(led_pin_Bypass, relayON);
    digitalWrite(relay_pin_Lock, LOW);
    digitalWrite(relay_pin_Switch, LOW);
    digitalWrite(relay_pin_Lock, !relayON);
    digitalWrite(relay_pin_Switch, relayON);
    relayCleanup.enableDelayed(8000);
    relayPartTwo.disable();
}

void resetRelay(){
	digitalWrite(relay_pin_Lock, LOW);
	digitalWrite(relay_pin_Switch, LOW);
  relayDisableIsolator.enableDelayed(5000);
	relayCleanup.disable();
}

void disableIsolator(){
  digitalWrite(relay_pin_Isolator, HIGH);
  relayDisableIsolator.disable();
}

 void midiMomentRelay(){
  if (!momentMode){
    flipRelay();
  }
  midiMomentaryMode.disable();
}
//###################################################
//           Preset Handling
//###################################################
void presetMode(){
  static bool blinkLight = false;
  presetModeFlag = true;
  digitalWrite(led_pin_Bypass, blinkLight);
  blinkLight = !blinkLight;
  if (digitalRead(switch_pin_Bypass) == HIGH){
    writePreset(timeDivision - 1);
    presetModeFlag = false;
    storePreset.disable();
  }
}
void readPreset(int presetNumber){
  EEPROM.readBlock(presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
  depth = constrain(currentPreset.depth, 0, 32);
  tapDivisor = constrain(currentPreset.multiplier, 0.5, 4.0);
  currentWaveTable = constrain(currentPreset.waveShape, 0, 4);
  stepRate = constrain(currentPreset.rate, minTapRate, maxTapRate);
  dutyCycle = constrain(currentPreset.spacing, 0.0625, 0.9375);
  floorOne = constrain(currentPreset.harmonicMixHigh, 0, 32);
  floorTwo = constrain(currentPreset.harmonicMixLow, 0, 32);
  synchronize = currentPreset.phase;
  splitDutyCycle(dutyCycle, applyTapDivision(stepRate));  
}

void writePreset(int presetNumber){
  digitalWrite(led_pin_Bypass, LOW);
  delay(1000);
  currentPreset = {depth, tapDivisor, currentWaveTable, stepRate, dutyCycle, floorOne, floorTwo, synchronize};
  EEPROM.writeBlock( presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
    for (int i = 0; i <= presetNumber; i++){
    digitalWrite(led_pin_Bypass, HIGH);
    delay(400);
    digitalWrite(led_pin_Bypass, LOW);
    delay(400);
    
  }
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
  setMidiChannel = true;
    midiChannel = MIDI_CHANNEL_OMNI;
    EEPROM.write(3, true);
    MIDI.setInputChannel(midiChannel);
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
	for (int i = numberOfBlinks; i > 0; i--){
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
  pinMode(relay_pin_Isolator, OUTPUT);
	pinMode(switch_pin_TapTempo, INPUT_PULLUP);
	pinMode(switch_pin_Mode,INPUT_PULLUP);
	pinMode(switch_pin_Phase,INPUT_PULLUP);
	pinMode(switch_pin_Bypass, INPUT_PULLUP);
  pinMode(midi_pin_Input, INPUT);
  
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
  
  switchMode = !digitalRead(switch_pin_Mode);
  momentMode = !switchMode;
  synchronize = digitalRead(switch_pin_Phase);
  
	attachInterrupt(digitalPinToInterrupt(switch_pin_TapTempo), tap, FALLING);

  
  midiOmni = EEPROM.read(3);
  if (midiOmni){
    midiChannel = MIDI_CHANNEL_OMNI;
  }
  else{
    midiChannel = EEPROM.read(2);
  }
  MIDI.begin(midiChannel);
  MIDI.turnThruOff();
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);  
  MIDI.setHandleContinue(handleContinue);
  
	oscillator.enable();
	checkControls.enable();
  updateMidi.enable();
}

void loop() {
ts.execute();
}
