# SPDX-License-Identifier: GPL-3.0-or-later
"""Original raw north-edge grid topology, flat/wrapped maps and RAM pixels."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
def fixture(profile):
 b=bytearray(valid_fixture(12,0,2));_,o,_=decode(b);struct.pack_into('<I',b,o['metadata']+28,profile&1)
 for i in range(24):b[o['tiles']+44*i]=(2 if(i+profile)%3==0 else 3)<<5
 return b
def model(b,o,q):
 x,y,enabled,prefs,sup=q
 if not sup or x<0 or y<0 or x>=8 or y>=6 or(x+y)&1:return bytes([0,0,0,0,99,99])
 land=b[o['tiles']+((y*8+x)//2)*44]>>5>=3;sea=bool(prefs&0x800000);flat=struct.unpack_from('<I',b,o['metadata']+28)[0];col=[0,0]
 if enabled and(land or sea)and y>0:
  for k in range(2):
   nx=x-1+k*2
   if not flat:nx=nx+8 if nx<0 else nx-8 if nx>=8 else nx
   if 0<=nx<8 and(b[o['tiles']+(((y-1)*8+nx)//2)*44]>>5>=3 or sea):col[k]=37 if land and b[o['tiles']+(((y-1)*8+nx)//2)*44]>>5>=3 else 190
 return bytes([1,0,0,0,*col])
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in['original_map','original_game','original_faction','original_world','terrain_mesh','terrain_line','terrain_grid','terrain_grid_probe']],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');digest=hashlib.sha256();count=0
 for profile in range(4):
  b=fixture(profile);_,o,_=decode(b);f=out/f'fixture{profile}.SC';f.write_bytes(b);qs=[(i%4*2+(i//4&1),i//4,e,p,1)for i in range(24)for e in[0,1]for p in[0,0x800000]]+[(0,0,1,0,0),(-1,0,1,0,1),(0,6,1,0,1),(1,0,1,0,1)];expected=b''.join(model(b,o,q)for q in qs);r=subprocess.run(runner+[str(exe),str(f)],input=b''.join(struct.pack('<5i',*q)for q in qs),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(profile,r.stdout,expected);digest.update(r.stdout);count+=len(qs);assert subprocess.run(runner+[str(exe),str(f)],input=b'partial',capture_output=True,env=env).returncode==20
 r=subprocess.run(runner+[str(exe)],capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==72*4096;assert digest.hexdigest()=='922bc26389585dbbdb252046600fd72df0cf63874ba2c67fdd865a66b38d7995';assert hashlib.sha256(r.stdout).hexdigest()=='e35147c70414b6738a3c00260154763d85cbdeb365813a721188a7315afa7995';record=dict(plans=count,planSHA256=digest.hexdigest(),layerCases=72,layerSHA256=hashlib.sha256(r.stdout).hexdigest(),worldAliasesPerFixture=27,layerAliasControls=4,layerInvalidOrLateFailureControls=5,partialRejected=True,bigEndian=a.m68k_linux,native=False);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
