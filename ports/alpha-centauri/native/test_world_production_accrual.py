#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Current accounting to original prefix, no freshness or phase authority."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_original_faction import SCHEMA as FACTIONS
from test_world_unit_stats import scalar
from test_production_accrual import model as prefix_model
HERE=Path(__file__).resolve().parent
NAMES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','production_accrual.cpp','world_production_accrual.cpp','world_production_accrual_probe.cpp']

def fixture(version,case):
 b=bytearray(valid_fixture(version,3,3));_,off,_=decode(b);old=version<11;bs=304 if old else 308;fs=0x20bc if old else 0x20cc
 for owner in range(8):struct.pack_into('<i',b,off['factions']+owner*fs+0x3f8,[-2147483648,-10,0,18,2147483647][(case+owner)%5])
 for i in range(3):
  p=off['bases']+i*bs;b[p+4]=(case+i)%8
  flags=[0,2,8,0x800000,0x800002,0xffffffff][(case+i)%6];values=[-2147483648,-9,-1,0,1,13,1073741823,1073741824,2147483647]
  for offset,fmt,value in [(0x30,'I',flags),(0xc0 if old else 0xc4,'i',values[(case+i)%len(values)]),(0xd0 if old else 0xd4,'i',values[(case*3+i)%len(values)]),(0x3c,'i',[-1,0,7][(case+i)%3]),(0x40,'i',[-2147483648,-1,0,111,2147483647][(case+i)%5]),(0x50,'i',[0,-38,-69,-70,-2147483648][(case+i)%5])]:struct.pack_into('<'+fmt,b,p+offset,value)
 return bytes(b)

def model(data,variant):
 info,_,s=decode(data);old=int(info['version'])<11;bs=304 if old else 308;fs=0x20bc if old else 0x20cc;schema=FACTIONS['LegacyFactionV10' if old else 'Faction'];result=[]
 for i in range(int(info['bases'])):
  b=s['bases'][i*bs:(i+1)*bs];faction=b[4];peak=scalar(s['factions'][faction*fs:(faction+1)*fs],schema,'best_mineral_output');flags=struct.unpack_from('<I',b,0x30)[0];mi=struct.unpack_from('<i',b,0xc0 if old else 0xc4)[0];ms=struct.unpack_from('<i',b,0xd0 if old else 0xd4)[0];food,stock=struct.unpack_from('<2i',b,0x3c);item=struct.unpack_from('<i',b,0x50)[0];result.append(prefix_model((int(variant=='expansion'),flags,mi,ms,food,stock,item,peak)))
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=out/'world-production-accrual-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 cases=[(f'v{v}-{c}','base' if v<11 else 'expansion',fixture(v,c)) for v in range(7,13) for c in range(40)]+[(f'v{v}-empty','base' if v<11 else 'expansion',valid_fixture(v,0,0)) for v in [7,12]]+[(p.name,'base',p.read_bytes()) for p in a.original];digest=hashlib.sha256();records=[];accepted=invalid=overflow=abort=accrual=0
 for name,variant,data in cases:
  p=out/(name+'.SC');p.write_bytes(data);results=model(data,variant);want=b''.join(struct.pack('<6I',*[x&0xffffffff for x in row]) for row in results);r=subprocess.run([*runner,str(exe),variant,str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);assert r.stdout==want,(name,list(struct.iter_unpack('<6I',r.stdout)),results);assert p.read_bytes()==data;digest.update(want);accepted+=sum(x[0]==0 for x in results);invalid+=sum(x[0]==1 for x in results);overflow+=sum(x[0]==2 for x in results);abort+=sum(x[0]==0 and x[4] for x in results);accrual+=sum(x[0]==0 and x[5] for x in results);(out/(name+'.bin')).write_bytes(r.stdout);records.append(dict(name=name,bases=len(results),inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest()))
 receipt=dict(worlds=len(cases),queries=accepted+invalid+overflow,accepted=accepted,invalid=invalid,overflow=overflow,negativeFoodAbort=abort,accrualBranches=accrual,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd,scope='Current owned accounting fields only, no fresh-yield or phase authority claim. Full decoded/opaque World byte snapshots and runtime RNG unchanged; aliases/empty/index/wrong edition/output transactional guards. No gameplay mutation/completion/turn/UI.');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
