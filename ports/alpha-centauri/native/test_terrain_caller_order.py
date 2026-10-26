# SPDX-License-Identifier: GPL-3.0-or-later
"""Caller gate and inherited light bounds; no opaque runtime defaults."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_terrain_observation import fixture
from test_original_game import decode

def model(w,o,q):
 x,y,v,draw,rules,edition,argument,lo,hi=q
 if x<0 or y<0 or x>=8 or y>=6 or (x+y)&1 or v< -1 or v>7 or edition not in [0,1] or lo< -256 or hi>256 or lo>hi:return [2]+[0]*6
 p=o['tiles']+((y*8+x)//2)*44;t=w[p:p+44];land=int(t[0]>>5>=3);ff=0 if v<0 else struct.unpack_from('<I',w,o['factions']+v*0x20bc)[0]
 if argument>21 or (not rules&0x100 and v>=0 and not ff&0x200 and not t[4]&(1<<v)):path=0
 elif draw&0x10000000:path=1
 elif draw&~1:return [3]+[0]*6
 else:path=2 if land else 3
 return [0,path,(-4 if land else lo)&0xffffffff,4 if land else hi,0xffffff00,256,land]

def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');a=a.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','terrain_caller_order','terrain_caller_order_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);w=fixture();_,o,_=decode(w);f=out/'fixture.SC';f.write_bytes(w);qs=[]
 for edition in range(2):
  for i in range(24):
   y=i//4;x=i%4*2+(y&1)
   for v in range(-1,8):
    for j in range(20):qs.append([x,y,v,[0,1,0x1000000,0x2000000,0x80000000,0x10000000,0x93000000,0,0x10,0x10000][j%10],0x100 if j&1 else 0,edition,[-1,0,21,22][j%4],-256 if j<8 else -4,256 if j<8 else 4])
 valid=len(qs)
 for pos,values in [(0,[-1,8]),(1,[-1,6]),(2,[-2,8]),(5,[-1,2]),(7,[-257,257]),(8,[-257,257])]:
  for n in values:q=[0,0,1,0,0,0,0,-256,256];q[pos]=n;qs.append(q)
 qs.append([1,0,1,0,0,0,0,-256,256]);qs.append([0,0,1,0,0,0,0,5,4]);data=b''.join(struct.pack('<9I',*[n&0xffffffff for n in q])for q in qs);expected=b''.join(struct.pack('<7I',*model(w,o,q))for q in qs);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];command=runner+[str(exe),str(f)];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(command,input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr)
 if r.stdout!=expected:
  for i,q in enumerate(qs):
   if r.stdout[i*28:(i+1)*28]!=expected[i*28:(i+1)*28]:raise AssertionError((i,q,struct.unpack_from('<7I',r.stdout,i*28),model(w,o,q)))
 assert subprocess.run(command,input=b'partial',capture_output=True,env=env).returncode==20
 digest=hashlib.sha256(expected).hexdigest();assert digest=='0824433c02b6662b171c4f4631665837e5f572cfd0999f79ca0c99cdc6af31e9',digest;(out/'output.bin').write_bytes(r.stdout);record=dict(synthetic=valid,invalid=len(qs)-valid,aliasRejects=10,missingRuntimeTransactional=True,partialRejected=True,editionProfiles=2,bigEndian=a.m68k_linux,sha256=digest,native=False);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
