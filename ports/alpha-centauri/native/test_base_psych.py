#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent integer model of pinned Thinker default psych restoration policy."""
import argparse,hashlib,itertools,json,os,random,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
SENTINEL=0x5A5A5A5A

def expected(row):
 """33 PsychFacts values:16numbers,3police,13bool,pending_economics."""
 assert len(row)==33
 pop,specialists,content,limit,index,bases,difficulty,capture,rule_drone,rule_talent,nodrone,talent,police,psych,genejack_drones,pacifism=row[:16]
 effects=row[16:19]
 genejack,rec,holo,hospital,nano,garden,transit,stapled,sphere,telepathic,longevity,genome,clinical=map(bool,row[19:32]);economics=row[32]
 fail=lambda code:[code]+[SENTINEL]*23
 if not(0<=pop<=127 and 0<=specialists<=pop and content>=0 and limit>=0 and 0<=index<512 and 1<=bases<=512 and 0<=difficulty<=5 and 0<=capture<=255 and rule_drone>=0 and rule_talent>=0 and -100<=talent<=100 and -100<=police<=100 and psych>=0 and 0<=genejack_drones<=100 and pacifism>=0):return fail(1)
 if any(not 0<=x<=4 for x in effects) or effects!=sorted(effects,reverse=True):return fail(1)
 can_riot=not(stapled or sphere or telepathic)
 if specialists==pop:return [0,*([0]*21),int(can_riot),0]
 fits=lambda x:-(1<<31)<=x<(1<<31)
 ordinary=pop-specialists
 # Keep an explicit list of citizens' aggregate categories; modify it through
 # an independent transcription of the source normalisation, preserving its
 # superdrone overlap behavior and recording each of the five display stages.
 state=[0,0,0];rows=[]
 def adjust(value=0,force=False):
  if force:
   for _ in range(max(0,value)):
    if state[0]<ordinary:state[0]+=1
    if sum(state)>ordinary:
     if state[2]>0:state[2]-=1
     elif state[1]>0:state[1]-=1
  elif value>0:state[0]+=value
  state[1]=max(0,min(pop,state[1]))
  while sum(state)>ordinary:
   if state[0]>0:
    state[0]-=1
    if state[2]>0:state[2]-=1
    elif state[1]>0:state[1]-=1
   elif state[1]<=ordinary:
    state[2]=max(0,min(state[1],state[2]));break
   else:state[1]-=1
 def record():rows.extend(state.copy())
 effic=0
 if limit:
  numerator=index%limit+bases-limit
  if not fits(numerator):return fail(5)
  effic=max(0,min(pop,numerator//limit))
 captured=max(0,min((pop+difficulty-2)//4,(capture+9)//10)) if capture>0 else 0
 drones=max(0,pop-content)+captured+(pop//rule_drone if rule_drone else 0)
 initial_talents=0
 if rule_talent:
  numerator=rule_talent+pop-1
  if not fits(numerator):return fail(5)
  initial_talents=numerator//rule_talent
 drones-=nodrone
 if not fits(drones):return fail(5)
 drones=max(0,drones)
 if talent>=0:initial_talents+=talent
 else:
  drones-=talent
  if not fits(drones):return fail(5)
 drones=max(0,min(pop,drones))+effic
 state[:]=[initial_talents,max(0,min(pop,drones)),max(0,min(pop,drones-pop))];adjust();record()
 first=max(0,min(psych//2,pop-state[0]));remaining=psych//2-first;adjust(first);step=2
 while remaining>=step and state[0]<ordinary:
  remaining-=step;step+=2;state[0]+=1;adjust()
 record()
 state[1]+=genejack_drones*genejack-2*rec-2*holo-int(transit and pop<=3)-hospital-nano
 if garden:adjust(2,True)
 adjust();record()
 police_total=0
 if police>=-1:
  slots=max(0,min(3,int(police==-1)+police+1));police_total=sum(effects[:slots]);state[1]=max(0,state[1]-police_total)
 extra=max(0,pacifism-1) if police==-3 else pacifism if police==-4 else 2*pacifism if police<=-5 else 0
 if not fits(extra) or not fits(state[1]+extra):return fail(5)
 if extra:state[1]=min(pop,state[1]+extra)
 adjust();record()
 if stapled or sphere:state[0]=state[1]=0;adjust()
 else:
  if longevity:
   if state[1]>0 and economics==2:state[1]-=1
   if state[1]>0 and economics!=1:state[1]-=1
  adjust(int(genome)+int(clinical),True)
 record()
 return [0,*state,effic,captured,police_total,*rows,int(can_riot),int(can_riot and state[1]>state[0])]

def fixture(pop=7):return [pop,0,2,5,3,7,3,0,0,0,0,0,0,0,2,0,2,1,0,*([0]*13),0]

def cases():
 rows=[]
 # Sweep the policy's branch points, each display-stage effect, the complete
 # signed-byte population domain and all ordinary specialist populations.
 for pop in range(128):
  for specialists in sorted({0,pop//2,pop}):
   for police in [-5,-4,-3,-2,-1,0,1,2,3]:
    row=fixture(pop);row[1]=specialists;row[12]=police;row[15]=3;rows.append(row)
 for flags in range(8192):
  row=fixture();row[19:32]=[(flags>>i)&1 for i in range(13)];row[7]=17;row[11]=-2;row[13]=flags%39;row[15]=flags%7;row[12]=flags%9-5;row[32]=flags%4;rows.append(row)
 for pop,psych,talent,content in itertools.product([1,3,7,20,127],[0,1,2,3,4,8,16,64,2147483647],[-100,-3,-1,0,1,3,100],[0,2,6,127,2147483647]):
  row=fixture(pop);row[13]=psych;row[11]=talent;row[2]=content;rows.append(row)
 for effects in itertools.product(range(5),repeat=3):
  row=fixture();row[16:19]=effects;rows.append(row)
 rng=random.Random(0x50535943)
 for _ in range(2000):
  pop=rng.randrange(128);row=fixture(pop);row[1]=rng.randrange(pop+1);row[2]=rng.randrange(8);row[3]=rng.choice([0,1,4,127,2147483647]);row[4]=rng.randrange(512);row[5]=rng.randrange(1,513);row[6]=rng.randrange(6);row[7]=rng.randrange(256);row[8]=rng.randrange(16);row[9]=rng.randrange(16);row[10]=rng.randrange(-16,17);row[11]=rng.randrange(-100,101);row[12]=rng.randrange(-100,101);row[13]=rng.choice([rng.randrange(100),2147483647]);row[15]=rng.choice([rng.randrange(100),2147483647]);row[16:19]=sorted([rng.randrange(5) for _ in range(3)],reverse=True);row[19:32]=[rng.randrange(2) for _ in range(13)];row[32]=rng.randrange(4);rows.append(row)
 for at,values in [(0,[-1,128]),(1,[-1,8]),(2,[-1]),(3,[-1]),(4,[-1,512,2147483647]),(5,[0,513]),(6,[-1,6]),(7,[-1,256]),(8,[-1,2147483647]),(9,[-1,2147483647]),(10,[-2147483648,2147483647]),(11,[-101,101,-2147483648,2147483647]),(12,[-101,101,-2147483648,2147483647]),(13,[-1]),(14,[-1,101]),(15,[-1]),(16,[-1,5])]:
  for value in values:row=fixture();row[at]=value;rows.append(row)
 return rows

def packet(rows):
 result=bytearray(struct.pack('<I',len(rows)))
 for row in rows:
  compact=[*row[:19],row[32],sum(bool(row[19+i])<<i for i in range(13))];result+=struct.pack('<21I',*(value&0xffffffff for value in compact))
 return bytes(result)

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path,required=True);parser.add_argument('--source-root',type=Path,default=HERE);parser.add_argument('--cc',default='c++');parser.add_argument('--binary',type=Path);args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=False)
 exe=args.binary.resolve() if args.binary else args.out/'base-psych-probe';command=[]
 if not args.binary:
  command=[args.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-I',str(args.source_root),str(args.source_root/'base_psych.cpp'),str(HERE/'base_psych_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 rows=cases();raw=packet(rows);(args.out/'cases.bin').write_bytes(raw);want=b''.join(struct.pack('<24I',*(v&0xffffffff for v in expected(row))) for row in rows);(args.out/'expected.bin').write_bytes(want)
 run=subprocess.run([str(exe)],input=raw,capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert run.returncode==0,(run.returncode,run.stderr)
 if run.stdout!=want:
  for i,row in enumerate(rows):
   got=run.stdout[96*i:96*(i+1)];target=want[96*i:96*(i+1)]
   if got!=target:raise AssertionError((i,row,list(struct.unpack('<24I',got)),expected(row)))
  raise AssertionError(('length',len(run.stdout),len(want)))
 (args.out/'actual.bin').write_bytes(run.stdout)
 receipt=dict(cases=len(rows),bytes=len(want),outputSHA256=hashlib.sha256(want).hexdigest(),accepted=sum(expected(row)[0]==0 for row in rows),allFiveStageRowsCompared=True,sourceAndOutputTransactionChecked=True,hostSanitizers=True,policy='Thinker default restoration policy dabe62f2d1abba03de40f3dd3048494843f6a9af',native='UNRUN',command=command);(args.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
