# SPDX-License-Identifier: GPL-3.0-or-later
# Independent integer model, validated against both actual original helpers.
import random
def bonus_model(seed,x,y,rules,climate,items):
 alt=climate>>5
 if not items&0x400:
  if not seed or (alt>2 and not rules&0x2000):return 0
 average=(x+y)//2;difference=x-average;code=(average&3)+(difference&3)*4
 if not items&0x400 and code!=(seed-5*((x+y)//8)-3*(difference//4))&15:return 0
 if alt<2:return 0
 code=code%3+1 if alt<3 else code%5&3
 if not code or items&0x20000000:code=3 if items&0x10000 else 2 if items&0x20000 else 1
 return code

def bonus_cases():
 rng=random.Random(20261029)
 cases=[]
 for alt in range(8):
  for seed in [0,1,16,0x7fffffff,0xffffffff]:
   for rules in [0,0x2000]:
    for items in [0,0x400,0x20000400,0x20010400,0x20020400]:
     for x,y in [(0,0),(2,0),(0,2),(1,1),(4,0),(0,4),(6,0),(0,6),(3,1),(1,3),(63,63),(62,0),(0,62),(17,31),(31,17),(34,62)]:cases.append((64,64,seed,x,y,rules,alt<<5,items,0))
 for _ in range(30000):
  width=rng.choice([2,4,16,64,128,4096]);height=rng.choice([1,2,16,64,4096]);x=rng.randrange(width);y=rng.randrange(height);x=(x&~1)|((y&1));items=rng.randrange(1<<30)
  cases.append((width,height,rng.getrandbits(32),x,y,rng.getrandbits(32),rng.randrange(256),items,0))
 return cases
