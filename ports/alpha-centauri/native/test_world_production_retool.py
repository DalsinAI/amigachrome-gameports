#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_production_retool import model,SRC
from test_original_map import word

def fixture(version,mode):
 b=bytearray(valid_fixture(version,3,2));info,o,_=decode(b);size=308 if version>=11 else 304;pref=o['preferences'] if version>=11 else o['legacy'];b[pref+0x58]=(1<<5) if mode&1 else 0
 for i in range(2):
  p=o['bases']+size*i;b[p+4]=5;struct.pack_into('<i',b,p+0x44,0 if i==0 else -10);struct.pack_into('<i',b,p+0x9c-(4 if version<11 else 0),111 if i==0 else 11);b[p+0x8c:p+0x8c+(12 if version>=11 else 8)]=bytes(12 if version>=11 else 8)
  if mode&2:b[p+0x8c+1]|=4
 return bytes(b)

def expected(data,profile,strict,penalty,exempt,proposed):
 info,o,_=decode(data);size=308 if int(info['version'])>=11 else 304;version=int(info['version']);pref=o['preferences'] if version>=11 else o['legacy'];h=2166136261
 for i in range(int(info['bases'])):
  p=o['bases']+size*i;fac=data[p+0x8c:p+0x8c+(12 if version>=11 else 8)].ljust(12,b'\0');faction=struct.unpack_from('<b',data,p+4)[0];human=bool(data[pref+0x58]&(1<<faction));old=struct.unpack_from('<i',data,p+0x44)[0];accum=struct.unpack_from('<i',data,p+0x9c-(4 if version<11 else 0))[0];r=model((profile,strict,penalty,exempt,human,old,proposed,accum,*struct.unpack('<3I',fac)));assert r[0]==0
  for x in [i,*r[1:]]:h=word(h,x)
 return dict(bases=str(info['bases']),digest=f'{h:08x}',unchanged='1',seed='abcdef01',rc='0')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 files=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','production_retool.cpp','world_production_retool.cpp','world_production_retool_probe.cpp'];exe=out/'world-retool-probe';cmd=[a.cc,*flags,*[str(SRC/f) for f in files],'-o',str(exe)];subprocess.run(cmd,check=True);records=[]
 for version in range(7,13):
  profile=int(version>=11);variant='expansion' if profile else 'base'
  for mode in range(4):
   data=fixture(version,mode);path=out/f'v{version}-{mode}.SC';path.write_bytes(data)
   for strict in range(4):
    for proposed in [-70,-39,-34,-10,-1,0,511]:
     args=[variant,str(path),str(strict),'50','10',str(proposed)];r=subprocess.run([*runner,str(exe),*args],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);want=expected(data,profile,strict,50,10,proposed);assert r.returncode==0 and dict(x.split('=') for x in r.stdout.split())==want,(version,mode,strict,proposed,r.returncode,r.stdout,r.stderr,want);records.append(dict(version=version,mode=mode,strictness=strict,proposed=proposed,passed=True,result=want))
   for args in [[('base' if profile else 'expansion'),str(path),'2','50','10','-1'],[variant,str(path),'4','50','10','-1'],[variant,str(path),'2','50','10','-2147483648']]:
    r=subprocess.run([*runner,str(exe),*args],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30);assert r.returncode==20 and 'unchanged=1' in r.stdout,(args,r.returncode,r.stdout,r.stderr);records.append(dict(rejection=args[0]+':'+args[2]+':'+args[-1],passed=True))
   assert path.read_bytes()==data
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(f'{len(records)} owned-world production retool cases passed')
if __name__=='__main__':main()
