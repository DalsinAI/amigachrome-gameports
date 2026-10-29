# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,json,struct,hashlib,subprocess,os
import test_local_unit_skip as skip
from test_units import sections,material
HERE=Path(__file__).resolve().parent
SOURCES=skip.SOURCES[:-1]+['world_unit_readiness.cpp','local_readiness.cpp','unit_readiness_probe.cpp']
def fixture(version,case):
 text,data,_,_=skip.fixture(version,0);t=sections(text);t['CHASSIS'][1][9]='2';text=material(t);b=bytearray(data);info,o,_=skip.resumed.workers.e.population.decode(b);id=int(info['vehicles'])-1;p=o['vehicles']+52*id
 if case<128:flags=[0,0x40,0x400,0x440][case%4];order=case//4%4;state=[0,0x200][case//16%2];site=case//32%4;carrier=0
 else:flags=0;order=1;state=0x4200;site=0;carrier=int(info['vehicles'])+1
 struct.pack_into('<HI',b,p+8,flags,64);struct.pack_into('<I',b,p+4,state);b[p+14]=1;b[p+16]=0;b[p+17]=order;b[p+36]=3;b[p+40]=[0,1,2][case%3];struct.pack_into('<h',b,p+20,carrier)
 struct.pack_into('<h',b,o['vehicles']+10,65);u=o['prototypes']+65*52;b[u:u+52]=bytes(52);b[u+36]=1;b[u+39]=1
 x,y=struct.unpack_from('<hh',b,p);tile=o['tiles']+(y*int(info['width'])+x)//2*44;items=struct.unpack_from('<I',b,tile+8)[0]&~0x40001;items|=1 if site in [1,2] else 0;items|=0x40000 if site==3 else 0;struct.pack_into('<I',b,tile+8,items);b[tile+2]=(b[tile+2]&0xf0)|(7 if site==2 else 0)
 # Readiness must inspect the *current repaired* map, so supply the site facts
 # through a real matching physical base for safe/Planet cases, not stale bits.
 bs=304 if version<11 else 308
 for j in range(int(info['bases'])):
  at=o['bases']+j*bs
  if j==0 and site in [1,2]:struct.pack_into('<hhb',b,at,x,y,0 if site==1 else 7)
  elif struct.unpack_from('<hh',b,at)==(x,y):struct.pack_into('<hh',b,at,0,0)
 # Keep an owner1 base alive away from queried site after moving base0 foreign.
 if site in [1,2]:b[o['bases']+bs+4]=1
 normalized=skip.resumed.normalized(text,bytes(b));_,o,_=skip.resumed.workers.e.population.decode(normalized)
 jailed=order==1 and site!=2 and site!=3;status=1 if case>=128 else 0
 bits=0;speed=0
 if not status and flags!=0x440:
  if jailed:bits=2
  elif not(order in [1,2,3] and not state&0x4200):speed=1;bits=4|int(speed>b[p+40])
 expected=bytearray(normalized)
 if bits&1:expected[p+40]=speed
 return text,bytes(b),bytes(expected),status,bits,speed

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in range(129):
   text,data,expected,status,bits,speed=fixture(version,case);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);(out/f'{i}.expected').write_bytes(expected);assert r.returncode==0 and r.stdout[20:]==expected and struct.unpack_from('<4I',r.stdout,4)==(status,bits,speed,0),(version,case,r.returncode,r.stderr,r.stdout[:20],status,bits,speed,next((j for j,(x,y) in enumerate(zip(r.stdout[20:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,status=status,bits=bits,guards=struct.unpack_from('<I',r.stdout)[0]))
 receipt=dict(worlds=len(records),accepted=sum(r['status']==0 for r in records),rejected=sum(r['status']!=0 for r in records),ready=sum(bool(r['bits']&1) for r in records),jailed=sum(bool(r['bits']&2) for r in records),guards=sum(r['guards'] for r in records),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
