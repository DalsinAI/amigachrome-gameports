#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(0x57f410)
inputs=bytearray();expected=bytearray();cases=[]
for v in range(2):
 for id in [0,2,63,64,65,127,320,511]:
  for mode in range(32):
   count=[0,1,2,7,16,32,64,512][mode%8];flags=rng.randrange(65536);header=struct.pack('<4I',v,id,flags,count);rows=[];answers=[];changed=id>=64 and bool(flags&1)
   for b in range(count):
    owner=rng.randrange(8);last=[-1,0,1,2,3,7,8,9][(mode+b)%8];items=[rng.choice([id,id,-69,-38,0,511]) for _ in range(10)]
    if mode&8:items=[id]*10
    rows.append(struct.pack('<12i',owner,last,*items));answer=items.copy();n=last
    if id>=64:
     j=0
     while j<=n:
      if answer[j]==id:
       changed=True
       if j==0:answer[j]=-38 if v==0 else -69
       else:
        for k in range(j,n):answer[k]=answer[k+1]
        n-=1
      j+=1
    answers.append(struct.pack('<12i',owner,n,*answer))
   inputs+=header+b''.join(rows);expected+=struct.pack('<4I',0,flags&~1 if id>=64 else flags,int(changed),count)+b''.join(answers);cases.append(dict(variant=v,id=id,count=count))
# Malformed queues reject the whole proposal, including earlier valid rows.
for last in [-2147483648,-2,10,2147483647]:
 inputs+=struct.pack('<4I',0,64,1,2)+struct.pack('<12i',1,0,64,*([0]*9))+struct.pack('<12i',2,last,*([64]*10));expected+=struct.pack('<I',1);cases.append(dict(reject=last))
(out/'inputs.bin').write_bytes(inputs);(out/'expected.bin').write_bytes(expected)
cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'prototype_retirement_queue.cpp'),str(src/'prototype_retirement_queue_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True)
r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));(out/'outputs.bin').write_bytes(r.stdout)
receipt=dict(cases=len(cases),bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(expected).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
