#!/usr/bin/env python3
"""Fixed 640x480 RGB chart for the target decoder and presentation checks."""
import struct
import sys
import zlib
from pathlib import Path

COLOURS = [(255,255,255),(0,0,0),(255,0,0),(0,255,0),(0,0,255),
           (255,255,0),(0,255,255),(255,0,255),(255,128,0),(128,0,255),
           (0,128,128),(128,64,0),(170,109,93),(255,207,162),(96,96,96),(192,192,192)]

def chunk(name, data):
    return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name+data))

def create(path):
    pixels = b''.join(b'\0' + b''.join(bytes(COLOURS[(x//80+(y//60)*3)%16]) for x in range(640)) for y in range(480))
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB',640,480,8,2,0,0,0)) + chunk(b'IDAT',zlib.compress(pixels,9)) + chunk(b'IEND',b''))

if __name__ == '__main__':
    create(sys.argv[1])
