# SPDX-License-Identifier: GPL-3.0-or-later
"""Viewer-specific original road/tube plans, raw base-bit repair and pixels."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
NAMES=['original_map','original_game','original_faction','original_world','terrain_mesh','terrain_forest_quad','terrain_roads','terrain_roads_probe']
def fixture(profile):
 b=bytearray(valid_fixture(12,0,2));_,o,_=decode(b)
 for v in range(8):struct.pack_into('<I',b,o['factions']+v*0x20cc,0x200 if v==profile+1 else 0)
 for i in range(24):
  p=o['tiles']+44*i;b[p]=(2 if i%5==profile else 3)<<5;b[p+4]=(i*37+profile*53)&255;flags=(4 if i%3 else 0)|(8 if(i+profile)%4==0 else 0)|(1 if i in[8,9]else 0);struct.pack_into('<9I',b,p+8,flags,flags^4,*[(flags&~12)|(4 if(i+v+profile)%3 else 0)|(8 if(i+v)%4==0 else 0)for v in range(1,8)])
 for i in range(2):p=o['bases']+i*308;b[p+4]=i+1;b[p+10]=0x5a
 if profile==4:struct.pack_into('<I',b,o['tiles']+8,1)
 return b

def model(b,o,q):
 x,y,v,items,sup=q
 if not sup or x<0 or y<0 or x>=8 or y>=6 or(x+y)&1 or v< -1 or v>7:return bytes([1,0,0,0]+[99]*10)
 def at(x,y):
  if not struct.unpack_from('<I',b,o['metadata']+28)[0]:
   if x<0:x+=8
   elif x>=8:x-=8
  if x<0 or y<0 or x>=8 or y>=6 or(x+y)&1:return None
  p=o['tiles']+((y*8+x)//2)*44;return x,y,b[p:p+44]
 def base(x,y,t):
  if not struct.unpack_from('<I',t,8)[0]&1:return -1
  for k in range(2):
   z=b[o['bases']+k*308:o['bases']+(k+1)*308];xx,yy=struct.unpack_from('<hh',z)
   if(xx,yy)==(x,y):return k if v<0 or z[4]==v or z[10]&(1<<v)else -1
  raise ValueError('repair')
 t=at(x,y)[2];visible=v<0 or struct.unpack_from('<I',b,o['factions']+v*0x20cc)[0]&0x200 or t[4]&(1<<v);out=[0]*9
 if not visible:return bytes([0,0,0,0]+out+[0])
 try:
  centre=base(x,y,t)
  if t[0]>>5>=3 and(items&4 or centre>=0):
   tube=items&8 or centre>=0;anyroad=anytube=False
   for k,(dx,dy)in enumerate([(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]):
    n=at(x+dx,y+dy)
    if not n or n[2][0]>>5<3:continue
    nx,ny,nt=n;flags=struct.unpack_from('<I',nt,8 if v<0 else 12+v*4)[0];nb=base(nx,ny,nt)
    if flags&4 or nb>=0:out[k]=2 if tube and(flags&8 or nb>=0)else 1;anyroad=True;anytube|=out[k]==2
   if centre<0 and(tube and not anytube or not anyroad):out[8]=2 if tube else 1
 except ValueError:return bytes([2,0,0,0]+[99]*10)
 return bytes([0,0,0,0]+out+[1])
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in NAMES],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');digest=hashlib.sha256();count=0
 for profile in range(5):
  b=fixture(profile);_,o,_=decode(b);f=out/f'fixture{profile}.SC';f.write_bytes(b);qs=[(i%4*2+(i//4&1),i//4,v,items,1)for i in range(24)for v in range(-1,8)for items in[0,4,12,0xffffffff]]+[(0,0,8,4,1),(1,0,1,4,1),(0,-1,1,4,1),(0,0,1,4,0)];data=b''.join(struct.pack('<5I',*[x&0xffffffff for x in q])for q in qs);expected=b''.join(model(b,o,q)for q in qs);r=subprocess.run(runner+[str(exe),str(f)],input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr)
  if r.stdout!=expected:
   for i,q in enumerate(qs):
    if r.stdout[i*14:(i+1)*14]!=expected[i*14:(i+1)*14]:raise AssertionError((profile,q,list(r.stdout[i*14:(i+1)*14]),list(expected[i*14:(i+1)*14])))
  digest.update(r.stdout);count+=len(qs);assert subprocess.run(runner+[str(exe),str(f)],input=b'partial',capture_output=True,env=env).returncode==20
 r=subprocess.run(runner+[str(exe)],capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==1024*4096;layer=hashlib.sha256(r.stdout).hexdigest();assert layer=='c177b5dae3297f4216f7433fad0e0025391a6a09bf8438c1ad3fd8ebe70496ea',layer;assert digest.hexdigest()=='d3e9e7ecc79b3468a553dd85b388ba1a4334a9350353425e8d92852f061f5bc6',digest.hexdigest();record=dict(planCases=count,planSHA256=digest.hexdigest(),layerCases=1024,layerSHA256=layer,worldAliasControls=24,layerAliasControls=10,layerInvalidOrLateFailureControls=9,lateExplorationGate=True,baseRepairHeld=True,native=False,bigEndian=a.m68k_linux);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
