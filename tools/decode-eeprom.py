#!/usr/bin/env python3
"""Decode Quaverato EEPROM Intel HEX (default presets image)."""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def parse_ihex(path: Path) -> bytes:
    mem: dict[int, int] = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line.startswith(":"):
            continue
        raw = bytes.fromhex(line[1:])
        count, addr, typ = raw[0], (raw[1] << 8) | raw[2], raw[3]
        if (sum(raw) & 0xFF) != 0:
            raise ValueError(f"bad checksum in {path}: {line}")
        if typ == 0:
            for i in range(count):
                mem[addr + i] = raw[4 + i]
        elif typ == 1:
            break
    if not mem:
        return b""
    size = max(mem) + 1
    return bytes(mem.get(i, 0xFF) for i in range(size))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "hexfile",
        nargs="?",
        default="eeprom/quaverato-default-presets.hex",
        type=Path,
    )
    args = ap.parse_args()
    data = parse_ihex(args.hexfile)
    print(f"file: {args.hexfile} ({len(data)} bytes)")
    print(f"addr0 schema = {data[0] if len(data) > 0 else 'n/a'}")
    print(f"addr1 bypass_flag = {data[1] if len(data) > 1 else 'n/a'}")
    print(f"addr2 midi_channel = {data[2] if len(data) > 2 else 'n/a'}")
    print(f"addr3 midi_omni = {data[3] if len(data) > 3 else 'n/a'}")
    print()
    print("#  depth   mult  wave        rate  spacing  hiMix  loMix  phase")
    root = 10
    for p in range(6):
        o = root + p * 19
        if o + 19 > len(data):
            break
        chunk = data[o : o + 19]
        depth = chunk[0]
        mult = struct.unpack_from("<f", chunk, 1)[0]
        wave = chunk[5]
        rate = struct.unpack_from("<I", chunk, 6)[0]
        spac = struct.unpack_from("<f", chunk, 10)[0]
        hi = struct.unpack_from("<h", chunk, 14)[0]
        lo = struct.unpack_from("<h", chunk, 16)[0]
        phase = chunk[18]
        print(
            f"{p}  {depth:5d} {mult:6.2f}    {wave} {rate:11d}   {spac:6.3f}  {hi:5d}  {lo:5d}    0x{phase:02X}"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
