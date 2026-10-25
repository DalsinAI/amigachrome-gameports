#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned ecology vs independent wire model and optional private original suffix."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_game import decode
from test_original_world import valid_fixture
from test_original_map import fixture as map_fixture
from worker_radius_reference import DX,DY
HERE=Path(__file__).resolve().parent
def corpus():
 rng=random.Random(20261027)
 for case in range(512):
  w=rng.choice([8,16,32]);h=rng.choice([6,12,20]);x=rng.randrange(w);y=rng.randrange(h);x=(x&~1)|(y&1);worked=rng.getrandbits(32);fac=(case>>1)&3;tiles=[rng.getrandbits(32) for _ in range(w//2*h)]
  if case<128:tiles=[0x200000 if case&8 else (0x1c8805c if case&16 else 0) for _ in tiles]
  yield dict(width=w,height=h,flat=case&1,x=x,y=y,worked=worked,facilities=fac,tiles=tiles)
def fixture(c,version):
 b=bytearray(valid_fixture(version,0,1));_,off,spans=decode(b);b[off['metadata']:off['abstract']+len(spans['abstract'])]=map_fixture(c['width'],c['height'])[15:];_,off,_=decode(b);p=off['bases'];bs=304 if version<11 else 308;b[p:p+bs]=bytes(bs);struct.pack_into('<hhb',b,p,c['x'],c['y'],1);struct.pack_into('<I',b,p+0x78,c['worked']);b[p+0x8d]=((c['facilities']&1)<<5)|(((c['facilities']>>1)&1)<<6);struct.pack_into('<I',b,off['metadata']+28,c['flat'])
 # Cached ecological and intake values deliberately poisoned; never inputs.
 struct.pack_into('<I',b,p+0x48,0xdeadbeef)
 for offset in [0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc]:struct.pack_into('<I',b,p+offset,0xcafebabe)
 for i,item in enumerate(c['tiles']):struct.pack_into('<I',b,off['tiles']+44*i+8,item)
 return bytes(b)
def wire_expected(data):
 info,off,spans=decode(data)
 if not int(info['bases']):return [1]+[0x5a5a5a5a]*23
 w,h=int(info['width']),int(info['height']);x,y=struct.unpack_from('<hh',spans['bases']);worked=struct.unpack_from('<I',spans['bases'],0x78)[0];fac=(spans['bases'][0x8d]>>5)&3;flat=struct.unpack_from('<I',spans['metadata'],28)[0];contributions=[];mask=0
 for i,(dx,dy) in enumerate(zip(DX[:21],DY[:21])):
  xx,yy=x+dx,y+dy
  if not flat:xx=xx+w if xx<0 else xx-w if xx>=w else xx
  if not(0<=xx<w and 0<=yy<h):contributions.append(0);continue
  mask|=1<<i;items=struct.unpack_from('<I',spans['tiles'],44*(yy*(w//2)+xx//2)+8)[0];n=(items&0x1c8805c).bit_count()*(2 if worked&(1<<i) else 1);n+=8*bool(items&0x1000000)+6*bool(items&0x800000)+4*bool(items&0x400000)-bool(items&0x200000)
  if fac==3:n=0
  if fac:n=(abs(n)//2)*(1 if n>=0 else -1)
  contributions.append(n)
 return [0,sum(contributions)&0xffffffff,mask,*[n&0xffffffff for n in contributions]]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-output',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','world_terraform_ecology.cpp','world_terraform_ecology_probe.cpp'];exe=a.out/'world-terraform-ecology-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in sources],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];digest=hashlib.sha256();original=bytearray();records=[]
 cases=[(str(i),fixture(c,7+i%6)) for i,c in enumerate(corpus())]
 for w,h in [(2,1),(2,2),(4,1),(4,3)]:
  for flat in [0,1]:cases.append((f'edge-{w}-{h}-{flat}',fixture(dict(width=w,height=h,flat=flat,x=0,y=0,worked=0xffffffff,facilities=0,tiles=[0x200000]*(w//2*h)),12)))
 first=next(corpus())
 for version in range(7,13):
  b=bytearray(fixture(first,version));_,off,_=decode(b);p=off['bases'];struct.pack_into('<i',b,p+0x50,-13);struct.pack_into('<I',b,p+0x48,0x12345678);b[p+0x8d]=0;cases.append((f'queued-tree-v{version}',bytes(b)))
 cases += [('empty',valid_fixture(12,0,0))]+[('owner-'+p.name,p.read_bytes()) for p in a.original]
 for name,data in cases:
  p=a.out/(name+'.SC');p.write_bytes(data);want=struct.pack('<24I',*wire_expected(data));r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);assert r.stdout==want,(name,r.stdout.hex(),want.hex());assert p.read_bytes()==data;digest.update(want)
  if name.isdigit():original+=want[4:8]
  records.append({'name':name,'inputSHA256':hashlib.sha256(data).hexdigest(),'outputSHA256':hashlib.sha256(want).hexdigest()})
 if a.original_output:assert bytes(original)==a.original_output.read_bytes()[:len(original)]
 (a.out/'original-totals.bin').write_bytes(original);receipt={'worlds':len(cases),'publicWorlds':527,'originalCompared':512 if a.original_output else 0,'wholeWorldAndWireUnchanged':True,'partialAliasesAndInvalidIDsAndUnloadedAndCoordinatesPerWorld':True,'originalTotalsSHA256':hashlib.sha256(original).hexdigest(),'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'cases':records,'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['cases','command']}))
if __name__=='__main__':main()
