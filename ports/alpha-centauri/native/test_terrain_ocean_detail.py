# SPDX-License-Identifier: GPL-3.0-or-later
"""Ocean contour detail, coast boundary exceptions and explicit light table."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','terrain_ocean_detail','terrain_ocean_detail_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');original=bytearray(valid_fixture());_,offsets,_=decode(original);start=offsets['tiles'];digest=hashlib.sha256();count=0;ds=[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)];light=[i%13-6 for i in range(80)];lp=out/'light.bin';lp.write_bytes(struct.pack('<80i',*light))
 def index(x,y):return start+(y*4+x//2)*44
 for fixture in range(16):
  b=bytearray(original);flat=fixture%2==1;struct.pack_into('<I',b,offsets['metadata']+28,int(flat))
  for y in range(6):
   for x in range(y&1,8,2):b[index(x,y)]=((x//2+y+fixture)%8)<<5;b[index(x,y)+1]=(x*17+y*29+fixture*23)%256
  def tile(x,y):
   if not flat:x%=8
   return None if x<0 or x>=8 or y<0 or y>=6 else(b[index(x,y)]>=96,b[index(x,y)+1])
  def detail(x,y,s,v):
   c=tile(x,y);n=c[1]
   if v==2:
    cx,cy=[(-1,0),(0,-1),(1,0),(0,1)][s];ns=[tile(x+cx+dx,y+cy+dy)for dx,dy in [(-1,0),(0,-1),(1,0),(0,1)]];n=sum(min(t[1],79)if t else 0 for t in ns)//4;n+=7 if ns[1]and ns[1][0]else 0;n-=3 if ns[3]and ns[3][0]else 0
   elif v%2:
    dx,dy=ds[(v+3+s*2)%8];t=tile(x+dx,y+dy);n=(min(c[1],79)+(min(t[1],79)if t else 0))//2
    dx,dy=ds[(s*2-4+(2 if v==3 else 0))%8];t=tile(x+dx,y+dy)
    if t and t[0]!=c[0]:
     low=(s in [0,3])if v==1 else(s in [2,3]);low=low!=c[0];return 57 if low else 67
   return max(0,min(79,n))
  path=out/'fixture.SC';path.write_bytes(b);want=bytes(t for y in range(6)for x in range(y&1,8,2)for s in range(4)for v in range(4)for d in [detail(x,y,s,v)]for t in [d,light[d]&255]);r=subprocess.run(runner+[str(exe),str(path),str(lp)],capture_output=True,env=env);assert r.returncode==0,(fixture,r.stderr);assert r.stdout==want,(fixture,next((i for i,(a,b)in enumerate(zip(r.stdout,want))if a!=b),None));digest.update(r.stdout);count+=24*16
  for x,y in [(-1,1),(1,0),(8,2),(0,6)]:
   r=subprocess.run(runner+[str(exe),str(path),str(x),str(y)],capture_output=True,text=True,env=env);assert r.returncode==0;assert list(map(int,r.stdout.split()))==[0]+[99]*16
 assert digest.hexdigest()=='7c73964417c1b89db11fd84815c50450f62f8c90a267215491e8e09ae1a4dfe8'
 record=dict(contexts=count,worldAliasChecksPerLoadedFixture=16,lightRejections=7,invalidQueries=64,bigEndian=a.m68k_linux,sha256=digest.hexdigest());(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
