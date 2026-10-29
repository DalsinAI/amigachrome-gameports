#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from tile_yield_reference import nutrient_cases,mineral_cases,energy_cases,nutrient_model,mineral_model,energy_model
HERE=Path(__file__).resolve().parent
class I32(int):
 def __new__(cls,n):
  if not -(1<<31)<=n<(1<<31):raise OverflowError(n)
  return int.__new__(cls,n)
 def __add__(self,n):return I32(int(self)+int(n))
 def __radd__(self,n):return I32(int(n)+int(self))
 def __sub__(self,n):return I32(int(self)-int(n))
 def __rsub__(self,n):return I32(int(n)-int(self))
 def __mul__(self,n):return I32(int(self)*int(n))
 def __rmul__(self,n):return I32(int(n)*int(self))
def bonus(climate,items):
 if not items&0x400 or climate>>5<2:return 0
 value=3 if climate>>5==2 else 2
 if items&0x20000000:value=3 if items&0x10000 else 2 if items&0x20000 else 1
 return value
def convert(edition,kind,c):
 row=[0]*59;row[0]=int(edition=='expansion');row[2]=5;row[14]=2;row[28]=1;row[31]=-123456
 if kind=='nutrient':
  climate,rough,items,lm,planet,fungus,project,events,fac,tech,exp,flag,res,mine=c
  row[3]=rough<<6;row[11]=fungus;row[20]=int(tech);row[27]=mine;channel=0
 elif kind=='mineral':
  climate,rough,items,lm,planet,fungus,project,events,fac,tech,sea_tech,exp,flag,aquatic,res,limit=c
  row[3]=rough<<6;row[12]=fungus;row[17]=int(aquatic);row[20]=int(tech)<<1|int(sea_tech)<<3;row[28]=limit;channel=1
 else:
  climate,items,lm,planet,fungus,project,events,fac,tech,exp,flag,res,economy,golden,count,governor,merchant,dust,solar,upkeep,neighbors=c
  row[10]=economy;row[13]=fungus;row[14]=count;row[15]=int(golden);row[20]=int(tech)<<2;row[21]=int(governor);row[22]=int(merchant);row[23]=int(dust);row[24]=solar;row[25]=int(upkeep);row[26]=sum(n==1 for n in neighbors);channel=2
 row[1]=climate;row[4]=items;row[5]=lm;row[6]=bonus(climate,items);row[7]=int(bool(exp) and edition=='expansion');row[8]=int(flag);row[9]=planet;row[16]=int(project);row[18]=fac&0xffffffff;row[19]=fac>>32;row[29]=events
 for j,value in enumerate(res):row[32+j*3+channel]=value
 return row
def expected(row):
 error=[1]+[0x5a5a5a5a]*9
 if row[0] not in [0,1] or row[1]>255 or row[2]>255 or row[3]>255 or row[20]&~15 or any(row[i] not in [0,1] for i in [7,8,15,16,17,21,22,23,25]):return error
 if (row[0]==0 and row[7]) or row[6]>3 or row[14]<0 or row[26]>8 or row[28]<0:return error
 try:
  r=row[:]
  for i in [9,10,11,12,13,14,27,28,30,31,*range(32,59)]:r[i]=I32(r[i])
  base=r[0]==0;climate,rough,it,lm=r[1],r[3]>>6,r[4],r[5]
  bs=bool(it&1 and r[2]&15<8)
  if not bs:it&=~1
  fac=r[18]|r[19]<<32
  common=[climate,rough,it,lm,r[9]];res=[[r[32+i*3+j] for i in range(9)] for j in range(3)]
  n=nutrient_model([*common,r[11],bool(r[16]),r[29],fac,bool(r[20]&1),bool(r[7]),r[8],res[0],r[27]],base,r[6])
  m=mineral_model([*common,r[12],bool(r[16]),r[29],fac,bool(r[20]&2),bool(r[20]&8),bool(r[7]),r[8],bool(r[17]),res[1],r[28]],base,r[6])
  e=energy_model([climate,it,lm,r[9],r[13],bool(r[16]),r[29],fac,bool(r[20]&4),bool(r[7]),r[8],res[2],r[10],bool(r[15]),r[14],bool(r[21]),bool(r[22]),bool(r[23]),r[24],bool(r[25]),[1]*r[26]],base,r[6],r[31])
  reductions=[I32(n[1]),I32(m[1]),I32(e[1])];counter=r[30]
  for delta in reductions:counter=I32(counter+delta)
  return [0,int(n[0]),int(m[0]),int(e[0]),int(counter),int(e[2]),*map(int,reductions),int(bs)]
 except OverflowError:return [2]+[0x5a5a5a5a]*9
def corpus():
 rows=[]
 for edition in ['base','expansion']:
  for kind,generate in [('nutrient',nutrient_cases),('mineral',mineral_cases),('energy',energy_cases)]:
   rows += [convert(edition,kind,c) for c in generate()]
 # Full triplets and actual record-domain/checked-arithmetic boundaries.
 import random
 rng=random.Random(20261028)
 for _ in range(4096):
  r=rows[rng.randrange(len(rows))][:]
  for i in range(32,59):r[i]=rng.randrange(-2,8)
  r[20]=rng.randrange(16);r[29]=rng.randrange(1<<15);r[30]=rng.randrange(-20,20);r[31]=rng.randrange(-20,20);r[6]=rng.randrange(4);r[2]=rng.randrange(256);rows.append(r)
 seed=rows[0][:]
 for i,n in [(0,2),(1,256),(2,256),(3,256),(6,4),(7,1),(8,2),(14,-1),(15,2),(16,2),(17,2),(20,16),(21,2),(22,2),(23,2),(25,2),(26,9),(28,-1)]:r=seed[:];r[i]=n;rows.append(r)
 for i in [9,10,11,12,13,27,30,31,*range(32,59)]:
  for n in [-(1<<31),(1<<31)-1]:r=seed[:];r[i]=n;rows.append(r)
 for n in [-(1<<31),(1<<31)-1]:
  for channel in range(3):
   r=seed[:];r[4]=1;r[6]=channel+1;r[32+3+channel]=n;r[15]=1;r[10]=n;rows.append(r)
 for edition in [0,1]:
  # Overflow before later clamps/caps is rejected, not silently wrapped.
  r=seed[:];r[0]=edition;r[4]=1;r[6]=1;r[35]=(1<<31)-1;rows.append(r)
  r=seed[:];r[0]=edition;r[4]=1;r[36]=(1<<31)-1;r[18]=1<<3;r[45]=1;rows.append(r)
  r=seed[:];r[0]=edition;r[4]=1;r[37]=-(1<<31);r[10]=-5;rows.append(r)
  r=seed[:];r[0]=edition;r[1]=0x60;r[4]=0x1000000;r[58]=(1<<31)-1;r[20]=4;r[24]=3;rows.append(r)
  r=seed[:];r[0]=edition;r[1]=0x60;r[4]=0x88000;r[47]=(1<<31)-1;r[20]=1;rows.append(r)
  r=seed[:];r[0]=edition;r[1]=0x60;r[4]=0x20;r[11]=-(1<<31);r[9]=-3;rows.append(r)
  r=seed[:];r[0]=edition;r[1]=0x60;r[4]=0x200000;r[41]=10;r[30]=(1<<31)-1;rows.append(r)
  r=seed[:];r[0]=edition;r[15]=1;r[10]=(1<<31)-1;rows.append(r)
 return rows
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 exe=a.out/'tile-yields-probe';flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])
 cmd=[a.cc,*flags,str(HERE/'tile_yields.cpp'),str(HERE/'tile_yields_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=corpus();data=b'ATYLD1\0\0'+struct.pack('<I',len(rows))+b''.join(struct.pack('<59I',*(n&0xffffffff for n in row)) for row in rows);source=a.out/'inputs.bin';output=a.out/'outputs.bin';source.write_bytes(data)
 runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];run=subprocess.run([*runner,str(exe),str(source),str(output)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(run.returncode,run.stderr)
 raw=output.read_bytes();assert len(raw)==len(rows)*40;totals=[0,0,0]
 for i,row in enumerate(rows):
  got=list(struct.unpack_from('<10i',raw,i*40));want=expected(row);assert got==want,(i,row,got,want);totals[got[0]]+=1
 assert source.read_bytes()==data
 receipt=dict(cases=len(rows),accepted=totals[0],invalid=totals[1],overflow=totals[2],inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(raw).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),nativeExecution='UNRUN',command=cmd)
 (a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
