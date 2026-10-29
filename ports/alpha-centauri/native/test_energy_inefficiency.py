#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checked original energy loss and nine proposed counters; synthetic inputs only."""
import argparse,hashlib,itertools,json,os,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
MIN,MAX=-(1<<31),(1<<31)-1
def word(n):return MIN<=n<=MAX
def expected(row):
 energy,eff,dist,creche,stage,*hist=row
 fail=lambda n:[n]+[0x5a5a5a5a]*10
 if not 0<=dist<999 or creche not in [0,1] or stage not in [0,1]:return fail(1)
 if energy<=0 or not dist:return [0,0,*hist]
 product=energy*dist
 if stage:
  if not word(product):return fail(3)
  hist=[v+(energy if j-2*creche>=8 else product//(64-8*j+16*creche)) for j,v in enumerate(hist)]
  if not all(map(word,hist)):return fail(3)
 if not word(-eff) or not word(4-eff):return fail(3)
 ineffective=4-eff-2*creche
 if ineffective>=8:loss=energy
 else:
  effective=8-ineffective;den=effective*8
  if not word(product) or not word(effective) or not word(den):return fail(3)
  loss=max(0,min(energy,product//den))
 return [0,loss,*hist]
def controls():
 rows=[]
 for energy,eff,creche,flat,stage,layout in itertools.product([-20,0,1,7,99,1000,100000],[-8,-4,-3,-2,0,2,4,8,10],[0,1],[0,1],[0,1],range(6)):
  hqs={0:[],1:[(6,6)],2:[(10,6)],3:[(30,6)],4:[],5:[(10,10),(20,14)]}[layout]
  distances=[]
  for x,y in hqs:
   dx,dy=abs(x-6),abs(y-6)
   if not flat and dx>16:dx=32-dx
   distances.append(max(dx,dy)-(((dx+dy)//2-min(dx,dy)+1)//2))
  rows.append([energy,eff,min(distances or [16]),creche,stage,*[j*17+(layout+eff)%13 for j in range(9)]])
 for energy,eff,dist,creche,stage in itertools.product([MIN,-1,0,1,MAX],[-8,MIN,MAX,0],[0,1,16,998],[0,1],[0,1]):rows.append([energy,eff,dist,creche,stage,*[MIN if j%2 else MAX for j in range(9)]])
 for index,value in [(2,-1),(2,999),(3,2),(4,2),(3,-1),(4,-1)]:
  row=[100,0,16,0,0,*range(9)];row[index]=value;rows.append(row)
 return rows
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'energy-inefficiency';flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];cmd=[a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(HERE/'energy_inefficiency.cpp'),str(HERE/'energy_inefficiency_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=controls();data=b'EIN1'+struct.pack('<I',len(rows))+b''.join(struct.pack('<14I',*[v&0xffffffff for v in row]) for row in rows);p=a.out/'controls.bin';p.write_bytes(data);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);want=b''.join(struct.pack('<11I',*[v&0xffffffff for v in expected(row)]) for row in rows);assert r.stdout==want,(len(r.stdout),len(want),next((i for i,(x,y) in enumerate(zip(r.stdout,want)) if x!=y),None));assert p.read_bytes()==data;(a.out/'outputs.bin').write_bytes(r.stdout)
 a0=sum(expected(row)[0]==0 for row in rows);bad=sum(expected(row)[0]==1 for row in rows);overflow=sum(expected(row)[0]==3 for row in rows);original=b''.join(r.stdout[44*i+4:44*(i+1)] for i in range(3024));receipt={'cases':len(rows),'accepted':a0,'invalid':bad,'overflow':overflow,'aliasesPerCase':3,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'original3024ProjectedOutputSHA256':hashlib.sha256(original).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN','scope':'Explicit numeric facts only; no HQ/world binding/accounting mutation/fullenergy/upkeep/turn.'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
