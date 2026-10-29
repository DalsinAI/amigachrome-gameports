#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
names=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','proposer_record_stages.cpp','proposer_record_stages_probe.cpp'];cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True)
vectors=bytearray();expected=bytearray();rows=[]
for active in [0,1]:
 for group in [-2147483648,-3,-2,-1,0,3,10,127,128,255,256,266,2147483647]:
  for difficulty in [-2147483648,1,2,2147483647]:
   for length in [0,3,31,32,141,255,256]:
    prior=bytearray((i*13+7)%256 for i in range(52));struct.pack_into('<H',prior,48,0xfffe|active);constructed=bytearray((i*7+3)%256 for i in range(52));struct.pack_into('<H',constructed,48,(length*29+group)&65535);name=(b'Z'*length+b'\0')[:256].ljust(256,b'Q');row=bytes(prior+constructed)+struct.pack('<ii',group,difficulty)+name;vectors+=row;rows.append(row)
    prepared=bytearray(constructed);flags=struct.unpack_from('<H',prepared,48)[0]|0x80|(0x40 if not active else 0)|(4 if group==-2 or difficulty<2 else 0);struct.pack_into('<H',prepared,48,flags)
    if group>=0:prepared[43]=group&255
    if group==10:prepared[42]=3
    expected+=struct.pack('<I',int(length==256))+prepared
    if length!=256:
     result=bytearray(prepared);result[:32]=(b'Z'*min(length,31)+bytes(32))[:32];struct.pack_into('<H',result,48,flags&~2);expected+=result
run=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe')];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(run,input=vectors,capture_output=True,env=env,timeout=120);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected));(out/'vectors.bin').write_bytes(vectors);(out/'outputs.bin').write_bytes(r.stdout)
world_outputs=bytearray()
# World wrappers bind actual pre-retirement active state; preserve all storage.
for version in [7,12]:
 for active in [0,1]:
  scene=bytearray(valid_fixture(version,3,2));_,off,_=decode(scene);struct.pack_into('<H',scene,off['prototypes']+64*52+48,active);path=out/(str(version)+'-'+str(active)+'.SC');path.write_bytes(scene)
  selected=[row for row in rows if (struct.unpack_from('<H',row,48)[0]&1)==active][:28];wr=bytearray();want=bytearray()
  for row in selected:
   n=rows.index(row);pos=sum(56+(52 if rows[j][112:368].find(b'\0')>=0 else 0) for j in range(n));size=56+(52 if row[112:368].find(b'\0')>=0 else 0);want+=expected[pos:pos+size];wr+=row
  r=subprocess.run(run+[str(path)],input=wr,capture_output=True,env=env,timeout=120);assert r.returncode==0,(version,active,r.returncode,r.stderr);assert r.stdout==want;world_outputs+=r.stdout
(out/'world-outputs.bin').write_bytes(world_outputs);receipt=dict(cases=len(rows),worldCases=112,allPass=True,aliases=13,unchangedWholeWorld=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(expected).hexdigest(),worldOutputSHA256=hashlib.sha256(world_outputs).hexdigest(),vectorsSHA256=hashlib.sha256(vectors).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(receipt)
