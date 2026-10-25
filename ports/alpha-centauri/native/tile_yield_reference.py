# SPDX-License-Identifier: GPL-3.0-or-later
# Independent integer models, checked against bounded actual original helpers.
import random

def nutrient_model(c,base=False,bonus_value=0,base_energy_before=-123456):
 climate,rough,items,landmarks,planet,fungus,project,events,fac,tech,exp,flag,res,mine=c
 sea=(climate&0xe0)<0x60;bonus=bonus_value==1
 base_sq=bool(items&1);landmark=((landmarks&0x80000004)==4 and not sea) or bool(landmarks&0x80 and sea)
 ocean,base_n,extra,forest,recycle,farm,kelp,monolith,bore=res
 cap=False
 if base_sq:
  value=base_n*(2 if bonus else 1)+int(landmark)
  if fac&((1<<3)|(1<<26)):value+=recycle
 elif items&0x1000000:
  value=bore+(extra if bore else 0) # actual original condition, not the later patched bonus predicate
 elif items&0x2000:value=monolith+(extra if bonus else 0)+int(not base and project and planet>0)
 elif items&0x20 and not flag and (climate&0xe0)>0x3f:value=max(0,min(99,fungus+min(0,max(-3,planet))))+int(not base and project and planet>0)
 else:
  cap=True
  if not sea:
   if items&0x200000:value=forest+(extra if bonus else 0)+int(bool(fac&(1<<14)))+int(bool(fac&(1<<13)))+int(landmark)
   else:
    value=0 if rough>=2 else (climate&0x18)>>3
    value=max(0,value+(extra if bonus else 0)+int(landmark))
    if items&0x8000 and rough<2:value+=farm
    if items&0x10 and value>1:value+=mine
  else:
   value=ocean+(extra if bonus else 0)+int(landmark)
   if (climate&0xe0)==0x40 or (not base and exp):
    if items&0x8000:value+=kelp+int(not base and bool(fac&(1<<36)))
    if items&0x10 and value>1:value+=mine
  if items&0x80000:value+=int(value/2)
  if items&0x400000:value+=int(value/2)
 reduction=0
 if cap and value>2 and not tech and not bonus and not items&0x400000:reduction=value-2;value=2
 if events&0x200:value+=1
 if events&0x400 and value:value-=1
 return value,reduction

def nutrient_cases():
 rng=random.Random(20261025)
 cases=[]
 for alt in [0x20,0x40,0x60,0x80,0xa0]:
  for item in [0,1,0x1000000,0x2000,0x20,0x200000,0x8000,0x10,0x80000,0x400000,0x88010,0x488010]:
   for bonus in [False,True]:
    for landmark in [0,4,0x80000004,0x80]:
     for tech in [False,True]:
      cases.append((alt|16,0,item|(0x20000400 if bonus else 0),landmark,1,2,True,0,(1<<3)|(1<<13)|(1<<14)|(1<<26)|(1<<36),tech,True,0,[1,2,2,1,1,1,1,2,0],-1))
 for _ in range(6000):
  items=rng.choice([0,1,0x1000000,0x2000,0x20,0x200000])
  for bit in [0x8000,0x10,0x80000,0x400000]:
   if rng.randrange(2):items|=bit
  if rng.randrange(2):items|=0x20000400
  fac=sum(1<<i for i in [3,13,14,26,36] if rng.randrange(2))
  cases.append((rng.choice([0x20,0x40,0x60,0x80,0xa0])|rng.choice([0,8,16]),rng.randrange(4),items,rng.choice([0,4,0x80000004,0x80]),rng.randrange(-5,6),rng.randrange(5),bool(rng.randrange(2)),rng.choice([0,0x200,0x400,0x600]),fac,bool(rng.randrange(2)),bool(rng.randrange(2)),rng.randrange(2),[rng.randrange(5) for _ in range(9)],rng.choice([-2,-1,0,1])))
 return cases

def mineral_model(c,base=False,bonus_value=0,base_energy_before=-123456):
 climate,rough,items,lm,planet,fungus,project,events,fac,tech,sea_tech,exp,flag,aquatic,res,roadlimit=c
 altitude=climate&0xe0;extra=bonus_value==2;lm_index=(lm if lm<0x80000000 else lm-0x100000000)>>24
 landmark=(((lm&0x80000002)==2 or (lm&0x80000001)==1) and lm_index<9) or (not base and (lm&0x80008000)==0x8000 and lm_index<6) or (lm&0x80000200)==0x200
 ocean,base_n,bonus_n,forest,recycle,unused,sea,monolith,bore=res
 value=(bonus_n if extra else 0)+int(landmark);reduction=0
 if items&1:
  value+=base_n
  if fac&((1<<3)|(1<<26)):value+=recycle
 elif items&0x2000:value=monolith+(bonus_n if extra else 0)+int(not base and project and planet>1)
 elif items&0x1000000:value=bore+(bonus_n if extra else 0)
 elif items&0x20 and altitude>0x3f:value=max(0,min(99,fungus+max(-3,min(0,planet))))+int(not base and project and planet>1)
 else:
  if altitude>=0x60:
   if items&0x200000:value+=forest
   elif not items&0x1000010 and not flag:value+=int(rough!=0)
   else:
    value+=rough;increment=max(1,rough)+int(extra or landmark)
    if not items&4 and not flag and increment>roadlimit:reduction+=increment-roadlimit;increment=roadlimit
    value+=increment
  else:
   value+=ocean
   if not base and aquatic and altitude==0x40:value+=1
   if (altitude==0x40 or (not base and exp)) and (items&0x10 or flag):value+=sea+int(sea_tech)+int(not base and bool(fac&(1<<37)))
  if value>2 and not tech and not extra:reduction+=value-2;value=2
 # Boreholes also pass the ordinary limit, unlike fungus/monolith/base.
 if items&0x1000000 and not items&(1|0x2000) and value>2 and not tech and not extra:reduction+=value-2;value=2
 if events&0x800:value+=1
 if events&0x1000 and value:value-=1
 return value,reduction

def mineral_cases():
 rng=random.Random(20261026)
 cases=[]
 for altitude in [0x20,0x40,0x60,0x80,0xa0]:
  for items in [0,1,0x2000,0x1000000,0x20,0x200000,0x10,0x14,0x8000,0x200014]:
   for rough in range(4):
    for code in [0,0x400,0x20000400,0x20020400,0x20010400]:
     for tech in [False,True]:cases.append((altitude|16,rough,items|code,2,2,3,True,0,(1<<3)|(1<<26)|(1<<37),tech,True,True,0,True,[1,1,2,2,1,0,1,2,6],1))
 for _ in range(10000):
  items=rng.choice([0,1,0x2000,0x1000000,0x20,0x200000])|rng.choice([0,0x400,0x20000400,0x20020400,0x20010400])
  for bit in [4,0x10,0x8000]:
   if rng.randrange(2):items|=bit
  lm=rng.choice([0,1,2,0x8000,0x200,0x80000002,0x80000001,0x80000200,0x80008000])|rng.choice([0,5<<24,6<<24,8<<24,9<<24])
  cases.append((rng.choice([0x20,0x40,0x60,0x80,0xa0])|rng.choice([0,8,16,24]),rng.randrange(4),items,lm,rng.randrange(-5,6),rng.randrange(5),bool(rng.randrange(2)),rng.choice([0,0x800,0x1000,0x1800]),sum(1<<b for b in [3,26,37] if rng.randrange(2)),bool(rng.randrange(2)),bool(rng.randrange(2)),bool(rng.randrange(2)),rng.randrange(2),bool(rng.randrange(2)),[rng.randrange(7) for _ in range(9)],rng.randrange(4)))
 return cases

def energy_model(c,base=False,bonus_value=0,base_energy_before=-123456):
 climate,items,lm,planet,fungus,project,events,fac,tech,exp,preview,res,economy,golden,base_count,governor,merchant,dust,solar,upkeep,neighbors=c
 alt=climate&0xe0;land=alt>=0x60;bs=bool(items&1);econ=economy+int(golden);reduction=0;terraform=base_energy_before
 lm_index=(lm if lm<0x80000000 else lm-0x100000000)>>24
 landmark=((lm&0x80000002)==2 and lm_index<9) or (lm&0x80000008)==8 or (lm&0x80000400)==0x400 or (lm&0x80000800)==0x800
 ocean,base_n,extra,forest,recycle,unused,sea,monolith,bore=res
 if bs:
  hq=bool(fac&2);value=base_n+int(hq)
  if fac&((1<<3)|(1<<26)):value+=recycle
  delta=econ
  if delta<0:
   if not hq or delta<-1 or base_count==1:delta+=1
  elif delta>1:delta=2*min(4,max(2,delta))-4
  value+=delta+int(governor)
 elif items&0x2000:value=monolith+int(not base and project and planet>=0)+int(not base and project and planet>2)
 elif items&0x1000000:value=bore
 elif items&0x20 and alt>0x3f:
  value=max(0,min(99,fungus+max(-3,min(0,planet))))+int(not base and project and planet>=0)+int(not base and project and planet>2)
  if events&0x2000:value+=1
  if events&0x4000 and value:value-=1
  if dust and value:value-=1
  return value*(3 if solar&(2 if upkeep else 1) else 1),0,terraform
 elif land:
  if items&0x200000:value=forest+int(bool(fac&(1<<14)))
  else:
   value=max(0,(climate>>5)-2) if items&0x1800040 or preview else 0
   if items&0x40:value+=sum(owner==1 for owner in neighbors)
 else:
  value=ocean+(sea+int(not base and bool(fac&(1<<38))) if items&0x40 or preview else 0) if alt==0x40 or (not base and exp) else 0
  if items&0x200000:value-=1
 if items&0x80 and land:value+=1
 be=bonus_value==3
 if be:value+=extra
 if landmark:value+=1
 if merchant:value+=1
 if econ>1:value+=1
 if not bs:
  if value>2 and not tech and not be:reduction=value-2;value=2
 else:value=max(0,min(99,value));terraform=value
 if events&0x2000:value+=1
 if events&0x4000 and value:value-=1
 if dust and value:value-=1
 value*=3 if solar&(2 if upkeep else 1) else 1
 return value,reduction,terraform

def energy_cases():
 rng=random.Random(20261027)
 cases=[]
 for alt in [0x20,0x40,0x60,0x80,0xa0]:
  for items in [0,1,0x2000,0x1000000,0x20,0x200000,0x40,0x800000,0x800040,0x80]:
   for code in [0,0x400,0x20000400,0x20020400,0x20010400]:
    for economy in range(-3,6):cases.append((alt|16,items|code,8,3,3,True,0,2|(1<<3)|(1<<14)|(1<<26)|(1<<38),False,True,0,[1,1,2,1,1,0,1,2,6],economy,False,2,True,True,False,0,False,[1,2,1,2,1,2,1,2]))
 for _ in range(10000):
  items=rng.choice([0,1,0x2000,0x1000000,0x20,0x200000,0x40,0x800000])|rng.choice([0,0x400,0x20000400,0x20020400,0x20010400])
  if rng.randrange(2):items|=0x80
  lm=rng.choice([0,2,8,0x400,0x800,0x80000002,0x80000008,0x80000400,0x80000800])|rng.choice([0,8<<24,9<<24])
  cases.append((rng.choice([0x20,0x40,0x60,0x80,0xa0])|rng.choice([0,8,16,24]),items,lm,rng.randrange(-5,6),rng.randrange(5),bool(rng.randrange(2)),rng.choice([0,0x2000,0x4000,0x6000]),sum(1<<b for b in [1,3,14,26,38] if rng.randrange(2)),bool(rng.randrange(2)),bool(rng.randrange(2)),rng.randrange(2),[rng.randrange(7) for _ in range(9)],rng.randrange(-5,7),bool(rng.randrange(2)),rng.choice([1,2,3]),bool(rng.randrange(2)),bool(rng.randrange(2)),bool(rng.randrange(2)),rng.randrange(4),bool(rng.randrange(2)),[rng.choice([0,1,2]) for _ in range(8)]))
 return cases
