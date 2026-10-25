#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent nutrient arithmetic and optional both-original record comparison."""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
def original_rows():
 rows=[]
 for pop in [0,1,2,127]:
  for rate in [-8,-1,0,7]:
   for flags in range(4):
    for stage in [0,1,2]:
     for stored in [-1,0,1]:rows.append([len(rows)%8,len(rows)%4,pop,rate,flags&1,flags>>1,[-10,0,2,100,1000][len(rows)%5],3,1,2,stored,stage,-10])
 rng=random.Random(0x4e9b70)
 for i in range(2048):rows.append([rng.randrange(8),rng.randrange(4),rng.randrange(128),rng.choice([-2147483648,-8,-1,0,8,2147483647]),rng.randrange(2),rng.randrange(2),rng.choice([-2147483648,-10,0,1,99,100,2147483647]),rng.choice([-1,0,1,2147483647]),rng.choice([-2147483648,-1,0,1,2147483647]),rng.choice([0,1,2,2147483647]),rng.choice([-2147483648,-10,-1,0,1,100,2147483647]),rng.randrange(3),rng.choice([-2147483648,-10,0,2147483647])])
 return rows
def facts(row):
 faction,id,pop,rate,creche,golden,ni,to,fr,req,stored,stage,total=row
 return [pop,rate,ni,to,fr,req,stored,total,stage,creche|(golden<<1)]
def expected(row):
 pop,rate,ni,to,fr,req,stored,total,stage,flags=row
 if not 0<=pop<=127 or req<0 or flags&~3:return [1]+[0x5a5a5a5a]*6
 def fit(n):
  if not -(1<<31)<=n<(1<<31):raise OverflowError
  return n
 try:
  growth=rate
  for mask in [1,2]:
   if flags&mask:growth=fit(growth+2)
  intake=fit(ni+to);consumption=fit(fit(pop*req)+fr);surplus=fit(intake-consumption)
  stored=0 if surplus>=0 and stored<0 else -1 if surplus<0 and stored==0 else stored
  if stage==1:total=fit(total+max(0,min(99,surplus)))
  return [0,growth,intake,consumption,surplus,stored,total]
 except OverflowError:return [3]+[0x5a5a5a5a]*6
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-control',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);rows=[facts(r) for r in original_rows()]
 seed=[2,0,10,0,0,2,0,0,0,0]
 for index,value in [(0,-1),(0,128),(5,-1),(9,4)]:r=seed[:];r[index]=value;rows.append(r)
 for stage in [3,0xffffffff]:r=seed[:];r[8]=stage;rows.append(r)
 data=b''.join(struct.pack('<10I',*[x&0xffffffff for x in r]) for r in rows);exe=a.out/'base-nutrients-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,str(HERE/'base_nutrients.cpp'),str(HERE/'base_nutrients_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==28*len(rows);accepted=invalid=overflow=0
 for i,row in enumerate(rows):
  want=expected(row);encoded=struct.pack('<7I',*[x&0xffffffff for x in want]);assert r.stdout[i*28:(i+1)*28]==encoded,(i,row,struct.unpack('<7I',r.stdout[i*28:(i+1)*28]),want);accepted+=want[0]==0;invalid+=want[0]==1;overflow+=want[0]==3
 original_comparisons={}
 if a.original_control:
  originals=original_rows()
  for edition in ['base','expansion']:
   c=a.original_control/('original-'+edition+'-final-2');inputs=(c/'inputs.bin').read_bytes();assert inputs==b''.join(struct.pack('<13i',*row) for row in originals);raw=(c/'outputs.bin').read_bytes();bs,fs,ni,cons,sur,tot=(304,0x20bc,0xbc,0xec,0xcc,0x7fc) if edition=='base' else (308,0x20cc,0xc0,0xf0,0xd0,0x800);size=bs+fs+4;assert len(raw)==len(originals)*size;count=0
   for i,row in enumerate(rows[:len(originals)]):
    want=expected(row)
    if want[0]:continue
    b=raw[i*size:(i+1)*size];got=[struct.unpack_from('<i',b,bs+fs)[0],*[struct.unpack_from('<i',b,j)[0] for j in [ni,cons,sur,0x3c]],struct.unpack_from('<i',b,bs+tot)[0]];assert want[1:]==got,(edition,i,want,got);count+=1
   original_comparisons[edition]=count
 (a.out/'controls.bin').write_bytes(data);(a.out/'results.bin').write_bytes(r.stdout);receipt={'cases':len(rows),'accepted':accepted,'invalid':invalid,'overflow':overflow,'directOriginalAcceptedComparisons':original_comparisons,'partialAliasesAndAdjacentControls':True,'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
