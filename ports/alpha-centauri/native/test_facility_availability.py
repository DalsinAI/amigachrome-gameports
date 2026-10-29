#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,copy,hashlib,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent

def encode(c):
 return struct.pack('<12i',*c['head'])+bytes(c['facilities'])+bytes(c['known'])+struct.pack('<64b',*c['projects'])+struct.pack('<10i',*c['queue'])+bytes([c.get('corrupt',0),0,0])

def expected(c):
 p,exp,caretaker,base,coast,faction,difficulty,count,rules,flags,i,pre=c['head']
 if p not in (0,1) or any(x not in (0,1) for x in (exp,caretaker,base,coast)) or not 1<=faction<=7 or not 0<=difficulty<=5 or not 0<=count<=10 or (p==0 and exp) or c.get('corrupt',0):return 1,1
 slots,first,repeat=(72,39,34) if p==0 else (134,70,65)
 if not 0<=i<slots or not -2<=pre<=88 or any(x not in (0,1) for x in c['known']) or any(not -2<=x<=7 for x in c['projects'][:slots-first]):return 1,1
 owned={j for j,v in enumerate(c['projects'][:slots-first]) if v==faction}
 built={j for j in range(1,repeat) if c['facilities'][j//8]&(1<<(j%8))}
 built|={-j for j in c['queue'][:count] if -repeat<j<0}
 yes=True
 if i==0 or (i==10 and difficulty<2) or (i>=first and rules&0x80000000):yes=False
 elif i==first+32:yes=c['projects'][31]!=-1 and bool(rules&0x800)
 elif pre!=-1 and (pre<0 or pre==88 or not c['known'][pre]):yes=False
 elif p and ((not exp and 103<=i<=106) or (caretaker and i==101)):yes=False
 elif i>=first:yes=c['projects'][i-first]==-1
 elif not base:yes=True
 elif i in built:yes=False
 elif i==28:yes=10 not in owned and bool(coast)
 elif i in {4:5,27:1,30:17,21:28,**({29:35,7:36} if p else {})} and {4:5,27:1,30:17,21:28,29:35,7:36}[i] in owned:yes=False
 elif p and ((not exp and 34<=i<=41) or (36<=i<=38 and not coast) or (i==40 and not flags&0x80)):yes=False
 elif i in {3,5,10,11,14,16,18,20,21,23,25,32}:
  yes={3:26 not in built,5:4 in built or 5 in owned,10:not flags&0x200,11:6 in built and 6 not in owned,14:13 in built,16:15 in built,18:17 in built,20:bool({19,22}&built),21:19 in built,23:12 not in built,25:24 in built,32:31 in built}[i]
 elif (34<=i<=37 if not p else 65<=i<=68 or i==41):yes=29 in built or 27 in owned or 35 in owned
 return 0,int(yes)

def contexts():
 rows=[]
 for p in (0,1):
  template={'head':[p,p,0,1,0,1,3,0,0,0,0,-1],'facilities':[0]*12,'known':[1]*89,'projects':[-1]*64,'queue':[0]*10}
  # Every ordinal with isolated dependency, technology, project and queue changes.
  for i in range(72 if p==0 else 134):
   variants=[{}]
   for j in range(1,33):variants.append({'facility':j})
   for j in (1,5,6,10,17,27,28,31,35,36):
    if not p and j>=33:continue
    for owner in (-2,1,2):variants.append({'project':(j,owner)})
   variants += [{'head':(k,v)} for k,v in [(1,0),(2,1),(3,0),(4,1),(5,7),(6,0),(8,-2147483648),(8,2048),(9,128),(9,512),(11,-2),(11,0),(11,88)]]
   variants += [{'queue':j} for j in (i,4,6,12,13,15,17,19,22,24,26,29,31)]
   for v in variants:
    c=copy.deepcopy(template);c['head'][10]=i
    if 'facility' in v:j=v['facility'];c['facilities'][j//8]|=1<<(j%8)
    if 'project' in v:j,o=v['project'];c['projects'][j]=o
    if 'head' in v:k,x=v['head'];c['head'][k]=x
    if 'queue' in v:c['head'][7]=1;c['queue'][0]=-v['queue']
    rows.append(c)
  rng=random.Random(1701+p)
  for _ in range(5000):
   c=copy.deepcopy(template);h=c['head'];h[1]=rng.randrange(2) if p else 0;h[2]=rng.randrange(2);h[3]=rng.randrange(2);h[4]=rng.randrange(2);h[5]=rng.randrange(1,8);h[6]=rng.randrange(6);h[7]=rng.randrange(11);h[8]=rng.choice([0,2048,-2147483648,-2147481600]);h[9]=rng.choice([0,128,512,640]);h[10]=rng.randrange(72 if not p else 134);h[11]=rng.randrange(-2,89)
   c['facilities']=[rng.randrange(256) for _ in range(12)];c['known']=[rng.randrange(2) for _ in range(89)];c['projects']=[rng.randrange(-2,8) for _ in range(64)];c['queue']=[-rng.randrange(72 if not p else 134) for _ in range(10)];rows.append(c)
  for k,v in [(0,2),(1,2),(2,2),(3,2),(4,2),(5,0),(5,8),(6,-1),(6,6),(7,-1),(7,11),(10,-1),(10,134),(11,-3),(11,89)]:
   c=copy.deepcopy(template);c['head'][k]=v;rows.append(c)
  for key,index,value in [('known',7,2),('projects',3,-3),('projects',4,8)]:
   c=copy.deepcopy(template);c[key][index]=value;rows.append(c)
  for value in range(1,5):c=copy.deepcopy(template);c['corrupt']=value;rows.append(c)
 return rows

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 rows=contexts();data=b''.join(map(encode,rows));want=b''.join(struct.pack('<2i',*expected(c)) for c in rows);(a.out/'controls.bin').write_bytes(data)
 binary=a.out/'probe';cmd=[a.cc,'-std=c++11','-Wall','-Wextra','-Werror','-O1','-I',str(HERE),str(HERE/'facility_availability.cpp'),str(HERE/'facility_availability_probe.cpp'),'-o',str(binary)]
 cmd+=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
 subprocess.run(cmd,check=True);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(binary),str(a.out/'controls.bin')],capture_output=True,env=env,check=True);(a.out/'results.bin').write_bytes(r.stdout)
 if r.stdout!=want:
  for i,c in enumerate(rows):
   got=r.stdout[i*8:i*8+8]
   if got!=want[i*8:i*8+8]:raise AssertionError((i,c,expected(c),got.hex()))
  raise AssertionError('length')
 receipt={'cases':len(rows),'accepted':sum(expected(c)[0]==0 for c in rows),'rejected':sum(expected(c)[0]!=0 for c in rows),'outputSHA256':hashlib.sha256(r.stdout).hexdigest()};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt))
if __name__=='__main__':main()
