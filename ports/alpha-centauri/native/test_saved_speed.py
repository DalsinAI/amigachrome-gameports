#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Saved-unit source-reference sweep and independent Python speed vectors."""
import argparse,hashlib,json,os,subprocess,struct
from pathlib import Path

def clamp(n,a,b):return min(b,max(a,n))
def calculate(row,table=None):
 variant,unit,chassis,weapon,reactor,abilities,plan,road,damage,options,morale=row
 if variant and unit==19:return [0,0]
 triad=0 if chassis<3 else 1 if chassis<5 else 2;transport=weapon==7;speed=chassis+1;offense=-1 if weapon==12 else 1
 if table is not None:
  triad=int(table[f'chassis.{chassis}.triad']);speed=int(table[f'chassis.{chassis}.speed']);transport=int(table[f'weapons.{weapon}.mode'])==7;offense=int(table[f'weapons.{weapon}.offense'])
 speed+=(reactor*2 if triad==2 else 0)-int(transport)-int(bool(abilities&0x800000))
 if abilities&0x400:speed+=reactor*2 if triad==2 else 1
 if triad==2:
  speed+=2*int(bool(abilities&0x8000000))+2*int(bool(options&1))
  if abilities&0x20:speed=speed*4//5
  if transport:speed//=2
 proto=clamp(speed,1,99)*road
 speed=proto+2*road*int(triad==1 and bool(options&2))
 speed+=road*int(not(options&8) and morale==6 and (unit>=64 or offense>=0))
 if damage and triad!=2:
  hp=1 if plan==12 else reactor*10;denominator=clamp(hp,1,99)
  # Equivalent integer ceiling, expressed independently of source numerator order.
  speed=clamp(-(-(speed//road*clamp(hp-damage,0,9999))//denominator),2 if triad==1 else 1,999)*road
 return [proto,speed]

def vectors():
 seed=0x83e14259;result=[]
 def rand(n):
  nonlocal seed
  seed=(seed*1664525+1013904223)&0xffffffff;return (seed>>8)%n
 flags=[0,0x400,0x800000,0x20,0x800420,0x8000000,0x8800420]
 for i in range(1024):
  variant=i&1;row=[variant,rand(512),rand(9),[0,7,12][rand(3)],1+rand(4),flags[rand(7 if variant else 5)],[3,12][rand(2)],[1,3,100][rand(3)],rand(256),rand(16),rand(7)];result.append(row+calculate(row))
 return result

def write_vectors(path):
 rows=vectors();path.parent.mkdir(parents=True,exist_ok=True);path.write_text('/* Independently generated synthetic controls; no game assets. */\nstatic const uint32_t speed_vectors[][13]={\n'+''.join('{'+','.join(f'0x{n:x}u' for n in row)+'},\n' for row in rows)+'};\n');return rows

def original_model(rules,data,variant):
 from test_original_game import decode
 from test_units import reference,sections
 from test_original_map import word
 table=reference(rules,variant);road=clamp(int(sections(rules)['RULES'][0][0]),1,100)
 info,_,spans=decode(data);assert (int(info['version'])<11)==(variant=='base')
 digest=2166136261;count=0
 for i in range(int(info['vehicles'])):
  v=spans['vehicles'][i*52:(i+1)*52];unit=struct.unpack_from('<h',v,10)[0];p=spans['prototypes'][unit*52:(unit+1)*52]
  for options in range(16):
   row=[int(variant=='expansion'),unit,p[36],p[37],p[39],struct.unpack_from('<I',p,32)[0],struct.unpack_from('<b',p,42)[0],road,v[16],options,6 if options&4 else 3]
   proto,speed=calculate(row,table)
   for value in [i,unit,options,proto,speed,2 if speed>255 else 0]:digest=word(digest,value)
   count+=1
 return dict(version=info['version'],vehicles=info['vehicles'],speed_contexts=str(count),digest=f'{digest:08x}',rc='0')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path);ap.add_argument('--vectors-only',type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 if a.vectors_only:write_vectors(a.vectors_only);return
 assert a.out is not None;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rows=write_vectors(out/'saved_speed_vectors.inc')
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-DALPHA_SPEED_VECTORS','-I'+str(out)];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'saved-speed-control';cmd=[a.cc,*flags,str(src/'saved_speed.cpp'),str(src/'saved_speed_control.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 assert r.returncode==0,(r.returncode,r.stdout,r.stderr);actual=dict(v.split('=') for v in r.stdout.split());expected=dict(independent='1024',saved_speeds='1492992',overflow='242586',invalid='12',digest='ac051c5b',rc='0');assert actual==expected,actual
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,result=actual,pythonVectorSHA256=hashlib.sha256(json.dumps(rows).encode()).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(r.stdout.strip())
if __name__=='__main__':main()
