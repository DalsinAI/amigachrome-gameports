# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent radius facts/model; no original executable or media data."""
import random
DX=(0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0)
DY=(0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4)
def corpus():
 rng=random.Random(20261031)
 for case in range(1280):
  width=rng.choice([8,16,32]);height=rng.choice([6,12,20]);flat=case&1;cx=rng.randrange(width);cy=rng.randrange(height);cx=(cx&~1)|(cy&1);reveal=bool(case&2);omniscient=bool(case&4);diplo=[0,1,3,0x8000008,0x800000b][case%5]
  tiles=[(rng.choice([0x40,0x60,0x80]),rng.randrange(16),rng.randrange(256),rng.choice([-1,0,1,2]),rng.choice([0,0,0,1])) for _ in range(width//2*height)]
  bases=[(cx,cy,1,0)]
  if case%3:
   bx=rng.randrange(width);by=rng.randrange(height);bx=(bx&~1)|(by&1);bases.append((bx,by,1 if case&8 else 2,rng.getrandbits(21)))
  actors=[]
  if case>=640:
   ax=cx if flat or case&16 else (cx+2)%width;ay=cy;actors=[(ax,ay,1 if case&32 else 2,3 if case&64 else 0,bool(case&128)),(ax,ay,2 if case&256 else 1,3 if case&512 else 0,bool(case&32))]
   i=ay*(width//2)+ax//2;alt,val,vis,owner,items=tiles[i];tiles[i]=(alt,val,vis,owner,items|2)
  yield dict(width=width,height=height,flat=flat,bases=bases,tiles=tiles,actors=actors,reveal=reveal,omniscient=omniscient,diplo=diplo)
def radius(c):
 width,height,flat=c['width'],c['height'],c['flat'];cx,cy,faction,_=c['bases'][0];bit=1<<faction
 diplo=c.get('diplomacy',[0,0,c.get('diplo',0),0,0,0,0,0])
 def wrap(x):return x if flat else x+width if x<0 else x-width if x>=width else x
 def valid(x,y):return 0<=x<width and 0<=y<height
 def tile(x,y):return c['tiles'][y*(width//2)+x//2]
 def distance(ax,ay,bx,by):
  x,y=abs(ax-bx),abs(ay-by)
  if not flat and x>width//2:x=width-x
  return max(x,y)-(((x+y)//2-min(x,y)+1)//2)
 flags=[]
 for i,(dx,dy) in enumerate(zip(DX,DY)):
  x,y=wrap(cx+dx),cy+dy
  if i>=21:flags.append(1);continue
  if not valid(x,y):flags.append(2);continue
  alt,val,visibility,owner,items=tile(x,y)
  if not c['reveal'] and not visibility&bit:flags.append(2);continue
  if owner>7:raise ValueError('unsafe owner')
  flag=4 if items&1 and val&15<8 else 0
  stack=[v for v in c['actors'] if v[0]==x and v[1]==y]
  if any(v[3]==3 or (v[2]!=faction and v[4] and not diplo[v[2]]&3) for v in stack):flag|=8
  territory=-1 if owner<1 or (owner!=faction and not c['omniscient'] and diplo[owner]&0x8000008!=0x8000008) else owner
  if territory>=0 and territory!=faction:flag|=32
  flags.append(flag)
 for bx,by,owner,worked in c['bases'][1:]:
  if distance(cx,cy,bx,by)>=6:continue
  for i in range(21):
   if i and not worked&(1<<i):continue
   x,y=wrap(bx+DX[i]),by+DY[i]
   if not valid(x,y) or (owner!=faction and tile(x,y)[0]>=0x60) or distance(cx,cy,x,y)>=3:continue
   for j in range(21):
    if wrap(cx+DX[j])==x and cy+DY[j]==y:flags[j]|=16
 return [sum(1<<i for i,f in enumerate(flags[:21]) if f),*flags]
