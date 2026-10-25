#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned original world lifetime, reload, resource and actor-reference controls."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import fixture,decode,SRC
from test_original_map import fnv

def valid_fixture(version=7,vehicles=3,bases=2):
 b=bytearray(fixture(version,vehicles=vehicles,bases=bases));_,off,_=decode(b)
 for i in range(vehicles):
  p=off['vehicles']+i*52;struct.pack_into('<hh',b,p,(i%4)*2,0);struct.pack_into('<h',b,p+10,(320+i)%512);b[p+14]=i%8
  struct.pack_into('<hhh',b,p+46,i%bases if bases else -1,-1,-1)
 for i in range(bases):
  p=off['bases']+i*(308 if version>=11 else 304);struct.pack_into('<hh',b,p,(i%4)*2,2);b[p+4]=i%8;struct.pack_into('<i',b,p+76,0)
 return bytes(b)

def model(data):
 info,_,_=decode(data)
 return dict(version=info['version'],vehicles=info['vehicles'],bases=info['bases'],tiles=str(int(info['width'])//2*int(info['height'])),bytes=str(len(data)),snapshot=f'{fnv(data):08x}',seed='abcdef01',ownership='1',transactional='1',rc='0')

def controls():
 result=[(f'owned-v{v}',valid_fixture(v),True,None) for v in range(7,13)]
 result += [('empty-actors',valid_fixture(12,0,0),True,None),('max-actors',valid_fixture(12,2048,512),True,None),('no-home-bases',valid_fixture(7,3,0),True,None)]
 data=valid_fixture();_,off,_=decode(data)
 def mutate(name,offset,fmt,value):
  b=bytearray(data);struct.pack_into('<'+fmt,b,offset,value);result.append((name,bytes(b),False,None))
 vp=off['vehicles'];bp=off['bases']
 for name,o,fmt,values in [('unit-x',0,'h',[-1,1,8]),('unit-y',2,'h',[-1,1,6]),('prototype',10,'h',[-1,512]),('unit-faction',14,'B',[8,255]),('home-base',46,'h',[-2,2]),('next-stack',48,'h',[-2,3]),('prev-stack',50,'h',[-2,3])]:
  for n in values:mutate(name+str(n),vp+o,fmt,n)
 for name,o,fmt,values in [('base-x',0,'h',[-1,1,8]),('base-y',2,'h',[-1,1,6]),('base-faction',4,'b',[-1,8]),('queue-size',76,'i',[-2,10])]:
  for n in values:mutate(name+str(n),bp+o,fmt,n)
 for limit in [0,1,4096]:result.append(('memory-limit'+str(limit),data,False,limit))
 for name,n in off.items():result.append(('short-'+name,data[:n],False,None))
 result += [('trailing',data+b'\0',False,None),('null-input',b'',False,None)]
 # Keep -1 queue sentinel, populated reciprocal stack references and owned state
 # even when two units share a tile. Import validates bounds, not ZOC/legality.
 b=bytearray(data);struct.pack_into('<i',b,bp+76,-1);result.append(('queue-sentinel',bytes(b),True,None))
 b=bytearray(data);struct.pack_into('<h',b,vp+48,1);struct.pack_into('<h',b,vp+52+50,0);result.append(('bounded-stack-links',bytes(b),True,None))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'world-probe';cmd=[a.cc,*flags,*[str(SRC/n) for n in ['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','original_world_probe.cpp']],'-o',str(exe)];subprocess.run(cmd,check=True)
 corpus=controls()+[('original-'+p.name,p.read_bytes(),True,None) for p in a.original];records=[]
 for i,(name,data,valid,limit) in enumerate(corpus):
  path=out/(str(i)+'.SC');path.write_bytes(data);args=[*runner,str(exe),str(path)]+([str(limit)] if limit is not None else [])
  r=subprocess.run(args,capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  expected=None
  if valid:
   expected=model(data);actual=dict(s.split('=') for s in r.stdout.split());assert actual==expected,(name,actual,expected)
  else:assert r.stdout.startswith('ERROR '),(name,r.stdout,r.stderr)
  records.append(dict(name=name,valid=valid,limit=limit,passed=True,sha256=hashlib.sha256(data).hexdigest(),result=expected))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(f'{len(records)} owned original-world cases passed')
if __name__=='__main__':main()
