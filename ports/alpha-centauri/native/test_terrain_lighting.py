# SPDX-License-Identifier: GPL-3.0-or-later
"""Procedural terrain brightness and actual owned-world point-light queries."""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def brightness(r):
 pairs=[(r[0]-4,r[1]-4),(4-r[2],r[1]-4),(4-r[2],4-r[3]),(r[0]-4,4-r[3])]
 def shade(x,y):
  x*=100;y*=100;d=math.isqrt(x*x+y*y);n=-3*(x+y)
  return (abs(n)//d*(-1 if n<0 else 1))if d else n
 return [shade(x,y)for x,y in pairs]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];names=['original_map','original_game','original_faction','original_world','cvr_terrain_context','terrain_lighting','terrain_lighting_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 want=bytes(v&255 for a in range(1,8)for b in range(1,8)for c in range(1,8)for d in range(1,8)for v in brightness([a,b,c,d]));r=subprocess.run(runner+[str(exe)],capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;(out/'brightness.bin').write_bytes(r.stdout)
 original=bytearray(valid_fixture());_,offsets,_=decode(original);start=offsets['tiles'];count=0;digest=hashlib.sha256()
 def index(x,y):return start+(y*4+x//2)*44
 corners=[[(-2,0),(-1,-1),(0,0),(-1,1)],[(-1,-1),(0,-2),(1,-1),(0,0)],[(0,0),(1,-1),(2,0),(1,1)],[(-1,1),(0,0),(1,1),(0,2)]]
 for fixture in range(12):
  b=bytearray(original)
  for y in range(6):
   for x in range(y&1,8,2):
    level=3 if fixture==0 else 3+(x//2+y+fixture)%2
    if fixture in [2,3]and x==3 and y==1:level=2
    if fixture==4 and x==4 and y==2:level=7
    b[index(x,y)]=level<<5;struct.pack_into('<I',b,index(x,y)+12,0)
  if fixture in [5,6]:struct.pack_into('<I',b,index(4,2)+12,0x200 if fixture==5 else 0x800)
  flat=fixture==7
  # Original map flags are stored at metadata+28 before the tile block.
  # Use decoded fixture's wrapped map; flat-map behavior gets its own fixture below.
  if flat:struct.pack_into('<I',b,offsets['metadata']+28,1)
  def tile(x,y):
   if not flat:x%=8
   if x<0 or x>=8 or y<0 or y>=6:return None
   return b[index(x,y)]>>5,struct.unpack_from('<I',b,index(x,y)+12)[0]
  def bright(x,y,k):
   t=tile(x,y)
   if t is None or t[0]<3:return 0
   if t[0]>6 or t[1]&0xb03:raise ValueError()
   relative=[]
   for group in corners:
    levels=[]
    for dx,dy in group:
     other=tile(x+dx,y+dy)
     if other is None or other[0]<3:relative.append(4);break
     if other[0]>6 or other[1]&0xb03:raise ValueError()
     levels.append(other[0])
    else:
     r=sum(levels)-4*t[0]+4
     if not 1<=r<=7:raise ValueError()
     relative.append(r)
   return brightness(relative)[k]
  path=out/'fixture.SC';path.write_bytes(b)
  for x,y in [(4,2),(0,0),(7,1),(-1,1),(1,0),(8,2)]:
   answer=[0]*6
   if 0<=x<8 and 0<=y<6 and(x+y)%2==0:
    try:
     if tile(x,y)[1]&0x203:raise ValueError()
     values=[sum(bright(x,y,k)for k in range(4))//4]
     for group in corners:values.append(sum(bright(x+dx,y+dy,(k+1+j)&3)for k,(dx,dy)in enumerate(group)for j in [0,1])//8)
     answer=[1,*values]
    except ValueError:pass
   r=subprocess.run(runner+[str(exe),str(path),str(x),str(y)],capture_output=True,text=True,env=env);assert r.returncode==0,(fixture,x,y,r.returncode,r.stderr);got=list(map(int,r.stdout.split()));assert got==answer,(fixture,x,y,got,answer);digest.update(struct.pack('<6i',*got));count+=1
 record=dict(brightnessCases=2401,worldCases=count,invalidCornerCases=8,worldAliasChecksPerLoadedFixture=16,bigEndian=a.m68k_linux,brightnessSHA256=hashlib.sha256(want).hexdigest(),worldSHA256=digest.hexdigest());(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
