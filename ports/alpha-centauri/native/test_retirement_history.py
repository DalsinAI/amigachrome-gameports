#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(0x57df30);inputs=bytearray();expected=bytearray();cases=0
for f in range(8):
 for cursor in range(64):
  table=bytearray(rng.randbytes(4096));model=rng.randbytes(8);inputs+=struct.pack('<3I',f,cursor,4096)+model+table;table[(f*64+cursor)*8:(f*64+cursor+1)*8]=model;expected+=struct.pack('<2I',0,(cursor+1)%64)+table;cases+=1
for f,c,n in [(-1,0,4096),(8,0,4096),(1,-1,4096),(1,64,4096),(1,0,0),(1,0,4095),(1,0,4097)]:
 inputs+=struct.pack('<3I',f&0xffffffff,c&0xffffffff,n)+bytes(8+4096);expected+=struct.pack('<2I',1,0x5a5a5a5a)+bytes([0x5a])*4096;cases+=1
(out/'inputs.bin').write_bytes(inputs);(out/'expected.bin').write_bytes(expected);cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'retirement_history.cpp'),str(src/'retirement_history_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected;(out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=cases,bigEndian68040=a.m68k_linux,allPass=True,outputSHA256=hashlib.sha256(expected).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
