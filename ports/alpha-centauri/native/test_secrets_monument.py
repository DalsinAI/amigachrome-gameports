# SPDX-License-Identifier: GPL-3.0-or-later
#!/usr/bin/env python3
from pathlib import Path
import sys,struct,random,json,hashlib,subprocess,os,argparse,shlex
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args()
SRC=Path(__file__).resolve().parent;OUT=Path(args.out).resolve();OUT.mkdir(parents=True,exist_ok=False);SIZE=0x27a0;STRIDE=0x4f4
rng=random.Random(0x476da0);records=[];expected=[]
for variant in range(2):
 for f in range(8):
  for mode in range(128):
   table=bytearray(rng.randbytes(SIZE));local=f if mode&1 else (f+1)%8;flags=128 if mode&2 else 0;year=[-1,0,2113,3200][mode%4]
   for j in range(8):
    achieved=0 if mode&8 else rng.choice([0,1,0xffffffff,0x80000000]);first=rng.choice([0,1,0xffffffff])
    if j==f:achieved=[0,1,0xffffffff,0x80000000][(mode//4)%4]
    struct.pack_into('<II',table,j*STRIDE+112+16,achieved,first)
   struct.pack_into('<II',table,f*STRIDE+116,*[0xffffffff if mode&(1<<(4+k)) else 2100+k for k in range(2)])
   if mode&64:struct.pack_into('<I',table,f*STRIDE+0x498,2)
   before=table[f*STRIDE:(f+1)*STRIDE];rec=bytearray(before);blocked=variant==1 and flags&128;presentation=False
   if not blocked:
    e=112
    achieved=struct.unpack_from('<I',rec,e+16)[0]
    if achieved:
     if struct.unpack_from('<I',rec,e+8)[0]==0xffffffff:struct.pack_into('<I',rec,e+8,year&0xffffffff)
     elif struct.unpack_from('<I',rec,e+4)[0]==0xffffffff:struct.pack_into('<I',rec,e+4,year&0xffffffff)
    else:
     others=[j for j in range(8) if j!=f and struct.unpack_from('<I',table,j*STRIDE+e+16)[0]]
     firsts=[j for j in others if struct.unpack_from('<I',table,j*STRIDE+e+20)[0]]
     struct.pack_into('<6I',rec,e,year&0xffffffff,0xffffffff,0xffffffff,firsts[-1] if firsts else f,1,int(not others))
    struct.pack_into('<I',rec,0x498,2);presentation=f==local
   h=[variant,f,local,2,year&0xffffffff,flags,SIZE]
   records.append(struct.pack('<7I',*h)+bytes(24)+table);expected.append(struct.pack('<3I',0,int(rec!=before),int(presentation))+rec)
inputs=b''.join(records);want=b''.join(expected);(OUT/'inputs.bin').write_bytes(inputs);(OUT/'expected.bin').write_bytes(want)
cross=args.m68k_linux;cmd=shlex.split(args.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(SRC/'monument_effects.cpp'),str(SRC/'monument_effects_probe.cpp'),'-o',str(OUT/'host-probe')];subprocess.run(cmd,check=True)
r=subprocess.run((['qemu-m68k','-cpu','m68040'] if cross else [])+[str(OUT/'host-probe'),str(OUT/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want,next((i for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b),None)
(OUT/'host-outputs.bin').write_bytes(r.stdout)
(OUT/'host-receipt.json').write_text(json.dumps(dict(cases=len(records),allPass=True,bigEndian68040=cross,inputSHA256=hashlib.sha256(inputs).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((OUT/'host-probe').read_bytes()).hexdigest(),command=cmd),indent=2)+'\n')
print('big-endian68040' if cross else 'host',len(records),hashlib.sha256(want).hexdigest())
