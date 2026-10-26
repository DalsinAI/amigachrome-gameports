# SPDX-License-Identifier: GPL-3.0-or-later
"""Original base-work grid with explicit cursor and raw using-faction semantics."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
def fixture(profile):
 b=bytearray(valid_fixture(12,0,2));_,o,_=decode(b);struct.pack_into('<I',b,o['metadata']+28,profile&1)
 for v in range(8):
  at=o['factions']+v*0x20cc;struct.pack_into('<I',b,at,0x200 if v==profile+1 else 0)
  for t in range(8):struct.pack_into('<I',b,at+0x18+t*4,8 if(v+t+profile)%3==0 else 0)
 for i in range(24):
  at=o['tiles']+44*i;b[at+4]=(i*37+profile*53)&255;b[at+5]=(b[at+5]&0xf8)|((i+profile)%8)
 return b
def model(b,o,q):
 x,y,v,cx,cy,sup=q
 if not sup or x<0 or y<0 or x>=8 or y>=6 or(x+y)&1 or v< -1 or v>7 or cx<0 or cy<0 or cx>=8 or cy>=6 or(cx+cy)&1:return bytes([0,0,0,0,99,0])
 t=o['tiles']+((y*8+x)//2)*44;flat=struct.unpack_from('<I',b,o['metadata']+28)[0]
 if v>=0 and not(struct.unpack_from('<I',b,o['factions']+v*0x20cc)[0]&0x200)and not(b[t+4]&(1<<v)):return bytes([1,0,0,0,0,0])
 def classify(x,y):
  t=o['tiles']+((y*8+x)//2)*44;user=b[t+5]&7
  if v>=0 and user!=v and not(struct.unpack_from('<I',b,o['factions']+v*0x20cc+0x18+user*4)[0]&8):user=0
  a=abs(x-cx);d=abs(y-cy)
  if not flat and a>4:a=8-a
  distance=max(a,d)-(((a+d)//2-min(a,d)+1)//2)
  return bool(user or distance<3)
 centre=classify(x,y);mask=0
 for k,(dx,dy)in enumerate([(1,-1),(1,1),(-1,1),(-1,-1)]):
  nx=x+dx;ny=y+dy
  if not flat:nx=nx+8 if nx<0 else nx-8 if nx>=8 else nx
  if 0<=nx<8 and 0<=ny<6 and classify(nx,ny)!=centre:mask|=1<<k
 return bytes([1,0,0,0,mask,1])
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in['original_map','original_game','original_faction','original_world','terrain_mesh','terrain_line','terrain_base_grid','terrain_base_grid_probe']],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');digest=hashlib.sha256();count=0
 for profile in range(4):
  b=fixture(profile);_,o,_=decode(b);f=out/f'fixture{profile}.SC';f.write_bytes(b);qs=[(i%4*2+(i//4&1),i//4,v,cx,cy,1)for i in range(24)for v in range(-1,8)for cx,cy in[(0,0),(4,2),(7,5)]]+[(0,0,1,0,0,0),(-1,0,1,0,0,1),(0,6,1,0,0,1),(1,0,1,0,0,1),(0,0,8,0,0,1),(0,0,1,-1,0,1),(0,0,1,8,0,1),(0,0,1,1,0,1)];expected=b''.join(model(b,o,q)for q in qs);r=subprocess.run(runner+[str(exe),str(f)],input=b''.join(struct.pack('<6i',*q)for q in qs),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(profile,r.stdout,expected);digest.update(r.stdout);count+=len(qs);assert subprocess.run(runner+[str(exe),str(f)],input=b'partial',capture_output=True,env=env).returncode==20
 r=subprocess.run(runner+[str(exe)],capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==128*4096;assert digest.hexdigest()=='cc13766acdd40980c3774abe15d391991a7991cf7562e24653faa66556dbf9d7';assert hashlib.sha256(r.stdout).hexdigest()=='fe50a2ab0e9a5243e2451722c55deffabb70ca4b0dfe64302822c478abc53075';record=dict(plans=count,planSHA256=digest.hexdigest(),layerCases=128,layerSHA256=hashlib.sha256(r.stdout).hexdigest(),worldAliasesPerFixture=27,rawBackedBunglistAndImportedMonumentsAliasCasesPerFixture=10,wholeRawAndSpanNoWriteChecks=True,currentRawOwnershipExplicitlyChecked=True,laterGameplayWorldIntegrationHeld=True,layerAliasControls=4,layerInvalidOrLateFailureControls=6,partialRejected=True,bigEndian=a.m68k_linux,native=False);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
