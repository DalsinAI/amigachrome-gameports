# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent ordinary/coast corner resolver controls; synthetic containers only."""
import argparse,os,struct,subprocess,json
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];names=['original_map','original_game','original_faction','original_world','cvr_terrain_context','cvr_terrain_context_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[]
 original=bytearray(valid_fixture());_,offsets,_=decode(original);start=offsets['tiles'];cases=0;accepted=0
 def index(x,y):return start+(y*4+x//2)*44
 corners=[[(0,0),(-1,-1),(-2,0),(-1,1)],[(0,0),(-1,-1),(0,-2),(1,-1)],[(0,0),(1,-1),(2,0),(1,1)],[(0,0),(1,1),(0,2),(-1,1)]]
 for fixture in range(10):
  b=bytearray(original)
  for y in range(6):
   for x in range(y&1,8,2):
    level=3 if fixture==0 else(3+(x//2+y+fixture)%2)
    if fixture in [2,3]and x==3 and y==1:level=2
    if fixture==4 and x==4 and y==2:level=7
    b[index(x,y)]=level<<5;struct.pack_into('<I',b,index(x,y)+12,0)
  if fixture==5:struct.pack_into('<I',b,index(4,2)+12,0x200)
  path=out/f'{fixture}.SC';path.write_bytes(b)
  for x,y,z in [(4,2,1),(4,2,2),(0,0,1),(7,1,2),(-1,1,1),(1,0,1),(4,2,3)]:
   answer=[0]*9
   if x>=0 and y>=0 and x<8 and y<6 and(x+y)%2==0 and z in [1,2]:
    centre=b[index(x,y)]>>5;flags=struct.unpack_from('<I',b,index(x,y)+12)[0]
    if 3<=centre<=6 and not flags&0xb03:
     valid=True;rels=[];heights=[];rh=(11 if z==1 else 21)*25//16
     for corner in corners:
      levels=[];shore=False
      for dx,dy in corner:
       xx=(x+dx)%8;yy=y+dy
       if yy<0 or yy>=6:shore=True;break
       level=b[index(xx,yy)]>>5
       if level<3:shore=True;break
       if level>6 or struct.unpack_from('<I',b,index(xx,yy)+12)[0]&0xb03:valid=False;break
       levels.append(level)
      if not valid:break
      rel=4 if shore else sum(levels)-4*centre+4
      if not 1<=rel<=7:valid=False;break
      rels.append(rel);heights.append(0 if shore else sum((k-3)*rh for k in levels)//4)
     if valid:answer=[1,*rels,*heights]
   r=subprocess.run(runner+[str(exe),str(path),str(x),str(y),str(z)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=20);assert r.returncode==0,(fixture,x,y,z,r.returncode,r.stderr);got=list(map(int,r.stdout.split()));assert got==answer,(fixture,x,y,z,got,answer);cases+=1;accepted+=answer[0]
 (out/'receipt.json').write_text(json.dumps(dict(cases=cases,accepted=accepted,rejected=cases-accepted,bigEndian=a.m68k_linux,independentCornerOracle=True,ownerAssets=False),indent=2)+'\n');print('TERRAIN CONTEXT',cases,accepted,'accepted',cases-accepted,'rejected PASS')
if __name__=='__main__':main()
