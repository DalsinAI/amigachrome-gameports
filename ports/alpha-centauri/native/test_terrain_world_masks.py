# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned neighbour-mask queries against independent byte-fixture interpretation."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','terrain_world_masks','terrain_world_masks_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');original=bytearray(valid_fixture());_,offsets,_=decode(original);start=offsets['tiles'];digest=hashlib.sha256();count=0
 directions=[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]
 def index(x,y):return start+(y*4+x//2)*44
 for fixture in range(12):
  b=bytearray(original);flat=fixture%2==1;struct.pack_into('<I',b,offsets['metadata']+28,int(flat))
  for y in range(6):
   for x in range(y&1,8,2):
    q=x//2+y+fixture;level=(q%7)+1;rough=(q//2)%4;b[index(x,y)]=(level<<5)|(rough<<3);struct.pack_into('<I',b,index(x,y)+8,0x20 if q%3 else 0);struct.pack_into('<I',b,index(x,y)+12,[0,4,0x80000004][q%3])
  def tile(x,y):
   if not flat:x%=8
   if x<0 or x>=8 or y<0 or y>=6:return None
   i=index(x,y);return b[i]>>5,(b[i]>>3)&3,struct.unpack_from('<I',b,i+8)[0],struct.unpack_from('<I',b,i+12)[0]
  path=out/'fixture.SC';path.write_bytes(b)
  for x,y in [(4,2),(0,0),(7,1),(-1,1),(1,0),(8,2)]:
   for mode in range(3):
    want=[0]*8
    if 0<=x<8 and 0<=y<6 and(x+y)%2==0 and mode!=2:
     level,rough,items,lm=tile(x,y);items=items if mode==0 else 0;land=level>=3;jungle=lm&0x80000004==4;coast=[0]*4;r=f=j=0
     for k,(dx,dy)in enumerate(directions):
      t=tile(x+dx,y+dy)
      if land and t and t[0]<3:
       if k%2:coast[(k//2+2)%4]|=2
       else:coast[(k//2+1)%4]|=4;coast[(k//2+2)%4]|=1
     if (rough or items&32 or jungle)if land else items&32:
      for k in range(8):
       dx,dy=directions[(k+7)%8];t=tile(x+dx,y+dy)
       if t:
        if t[2]&32 and t[0]>=2:f|=1<<k
        if land:
         if jungle and t[3]&0x80000004==4 and t[0]>=3:j|=1<<k
         if rough and(t[0]<3 or t[1]>=rough):r|=1<<k
     want=[1,*coast,r,f,j]
    r=subprocess.run(runner+[str(exe),str(path),str(x),str(y),str(mode)],capture_output=True,text=True,env=env);assert r.returncode==0,(fixture,x,y,mode,r.stderr);got=list(map(int,r.stdout.split()));assert got==want,(fixture,x,y,mode,got,want);digest.update(bytes(got));count+=1
 assert digest.hexdigest()=='459d47c4b20bb94c59281512a722c649e85dc61041363edc260a8e10f85047a8'
 record=dict(cases=count,worldAliasChecksPerFixture=18,missingViewTransactional=True,bigEndian=a.m68k_linux,sha256=digest.hexdigest());(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
