#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,itertools,json,os,random,struct,subprocess
SRC=Path(__file__).resolve().parent

def contexts():
 rng=random.Random(2026102609);rows=[]
 for v in range(2):
  for i,choices in enumerate(itertools.product(range(4),repeat=4)):
   h=[v,*choices,*[rng.randrange(-7,8) for _ in range(11)],0,*([0]*24),128 if i&1 else 0,i%32]
   rows.append(h+[rng.randrange(-4,5) for _ in range(176)])
  for i in range(2048):
   count=i%9;kinds=[];first=[];second=[]
   for j in range(8):
    k=rng.choice([0,1,4,5,6,8,10,18]);kinds.append(k);first.append(rng.randrange(11) if k in [4,10] else rng.randrange(4) if k in [5,6] else rng.randrange(-30,31));second.append(rng.randrange(4) if k in [5,6] else rng.randrange(-100,101))
   rows.append([v,*[rng.randrange(4) for _ in range(4)],*[rng.randrange(-15,16) for _ in range(11)],count,*kinds,*first,*second,rng.getrandbits(32),rng.randrange(32),*[rng.randrange(-7,8) for _ in range(176)]])
 good=[0,*([0]*4),*([0]*11),0,*([0]*24),0,0,*([0]*176)]
 for field,values in [(0,[2,0xffffffff]),(1,[4,0xffffffff]),(16,[-1,9])]:
  for value in values:
   h=good.copy();h[field]=value;rows.append(h)
 for k in [4,5,6,10]:
  for bad in [-1,11]:
   h=good.copy();h[16]=1;h[17]=k;h[25]=bad;rows.append(h)
 for k in [5,6]:
  for bad in [-1,4]:
   h=good.copy();h[16]=1;h[17]=k;h[33]=bad;rows.append(h)
 for k in [-1,19]:
  h=good.copy();h[16]=1;h[17]=k;rows.append(h)
 # Current base modifiers remain authoritative even when SOCIAL bonuses
 # disagree. Exercise full clamp range and ordered odd-negative robust effects.
 for v in range(2):
  for value in [-2147483648,-999,-3,-1,0,999,1000,2147483647]:
   for effect in [0,5,10]:
    for kinds in [[],[4],[10],[4,10],[10,4],[10,10],[3]]:
     h=good.copy();h[0]=v;h[5+effect]=value;h[16]=len(kinds)
     for i,k in enumerate(kinds):h[17+i]=k;h[25+i]=effect;h[33+i]=123
     rows.append(h)
 # Overflow must reject even when a later IMMUNITY would clamp the result.
 for sign in [-1,1]:
  for k in range(11):
   h=good.copy();h[5+k]=2147483647 if sign>0 else -2147483648;h[43+k]=sign;h[16]=1;h[17]=4;h[25]=k;rows.append(h)
 return rows

def model(h):
 v=h[0];sel=h[1:5];base=h[5:16];count=h[16];kinds=h[17:25];first=h[25:33];second=h[33:41];rules=h[43:];mask=h[42]
 bad=lambda e:struct.pack('<I',e)+bytes([90])*152
 if v not in [0,1] or any(not 0<=x<4 for x in sel) or not 0<=count<=8:return bad(1)
 for k,a,b in zip(kinds[:count],first[:count],second[:count]):
  if not 0<=k<=18 or k in [4,10] and not 0<=a<11 or k in [5,6] and (not 0<=a<4 or not 0<=b<4):return bad(1)
 result=[0]*11
 def add(i,x):
  result[i]+=x
  if not -2147483648<=result[i]<=2147483647:raise OverflowError
 try:
  for cat in range(4):
   for eff in range(11):
    value=rules[cat*44+sel[cat]*11+eff]
    if value<0:
     if cat==3 and (sel[cat]==1 and mask&1 or sel[cat]==3 and mask&2) or cat==2 and sel[cat]==1 and mask&2:continue
     for k,a,b in zip(kinds[:count],first[:count],second[:count]):
      if a==cat and b==sel[cat]:
       if k==5:add(eff,-value)
       if k==6:add(eff,value)
    add(eff,value)
  if mask&4:add(5,1)
  if mask&8:add(2,2)
  if mask&16:
   add(7,1)
   if v and h[41]&128:add(10,1)
  for i,value in enumerate(base):add(i,value)
 except OverflowError:return bad(2)
 for k,a in zip(kinds[:count],first[:count]):
  if k==4:result[a]=max(0,min(999,result[a]))
  elif k==10 and result[a]<0:result[a]=-((-result[a])//2)
 return struct.pack('<39I',0,*sel,*[x&0xffffffff for x in result*3],0)

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 rows=contexts();raw=b''.join(struct.pack('<219I',*[x&0xffffffff for x in h]) for h in rows);want=b''.join(model(h) for h in rows);(out/'inputs.bin').write_bytes(raw)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,str(SRC/'social_upkeep.cpp'),str(SRC/'social_upkeep_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(out/'inputs.bin')],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want; (out/'outputs.bin').write_bytes(r.stdout)
 errors=[struct.unpack_from('<I',want,i*156)[0] for i in range(len(rows))];receipt=dict(cases=len(rows),accepted=errors.count(0),invalid=errors.count(1),overflow=errors.count(2),inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
