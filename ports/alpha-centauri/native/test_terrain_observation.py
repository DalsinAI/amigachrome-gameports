# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned saved-tile observation prelude; complex actor visibility stays held."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def fixture():
 b=bytearray(valid_fixture());_,o,_=decode(b)
 for i in range(8):struct.pack_into('<I',b,o['factions']+i*0x20bc,0x200 if i%3==0 else 0)
 for i in range(24):
  p=o['tiles']+i*44;b[p]=(i*11)%256;b[p+4]=(i*37)%256;b[p+5]=(i*67)%256
  struct.pack_into('<9I',b,p+8,0x8a005520^i,0x123a4321^i,*[0xabc00000+j*0x1234+i for j in range(7)])
 return b

def model(w,q,o):
 x,y,v,z,cur,draw,rules,prefs,state,edition=q
 if x<0 or y<0 or x>=8 or y>=6 or (x+y)&1 or v< -1 or v>7 or z< -16 or z>16 or cur<0 or cur>7 or edition not in [0,1]:return [4]+[0]*8
 p=o['tiles']+((y*8+x)//2)*44;t=w[p:p+44];ff=0 if v<0 else struct.unpack_from('<I',w,o['factions']+v*0x20bc)[0];items,lm=struct.unpack_from('<II',t,8)
 if v>=0 and not ff&0x200:items=lm if v==0 else struct.unpack_from('<I',t,12+4*v)[0]
 rain=(t[0]>>3)&3;rock=t[5]>>6;sf=v if v>=0 else (t[5]&7 or cur);far=int(z< -6 or bool(draw&0x80000000));conceal=0
 if draw&0x1000000:rain=rock=items=0;conceal=1
 elif rules&0x100 and not state&0x80:
  if v<0:return [2]+[0]*8
  if not ff&0x200 and not t[4]&(1<<v):return [0,items&~0x20,0,0,sf,1,0,lm|0x200000,far]
 if not draw&0x1000000 and not(state&0x80 or v<1 or not prefs&1 or z< -7):return [1]+[0]*8
 return [0,items,rain,rock,sf,conceal,1,lm&~0x200000,far]

def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');a=a.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','terrain_observation','terrain_observation_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);w=fixture();_,offsets,_=decode(w);f=out/'fixture.SC';f.write_bytes(w);qs=[]
 for edition in range(2):
  for i in range(24):
   y=i//4;x=i%4*2+(y&1)
   for v in range(-1,8):
    for j in range(16):qs.append([x,y,v,[-16,-9,-8,-7,-6,0,1,16][j%8],j%8,[0,0x1000000,0x80000000,0][j%4],0x100 if j&1 else 0,j//2&1,0x80 if j&8 else 0,edition])
 valid=len(qs)
 for pos,values in [(0,[-1,8]),(1,[-1,6]),(2,[-2,8]),(3,[-17,17]),(4,[-1,8]),(9,[-1,2])]:
  for n in values:q=[0,0,1,0,1,0,0,0,0,0];q[pos]=n;qs.append(q)
 qs.append([1,0,1,0,1,0,0,0,0,0]);data=b''.join(struct.pack('<10I',*[n&0xffffffff for n in q])for q in qs);expected=b''.join(struct.pack('<9I',*model(w,q,offsets))for q in qs);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];command=runner+[str(exe),str(f)];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(command,input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr)
 if r.stdout!=expected:
  for i,q in enumerate(qs):
   if r.stdout[i*36:(i+1)*36]!=expected[i*36:(i+1)*36]:raise AssertionError((i,q,struct.unpack_from('<9I',r.stdout,i*36),model(w,q,offsets)))
 assert subprocess.run(command,input=b'partial',capture_output=True,env=env).returncode==20
 counts={str(status):sum(model(w,q,offsets)[0]==status for q in qs)for status in range(5)};digest=hashlib.sha256(expected).hexdigest();assert digest=='75a4f9ea317c7a54efa738d7300181229a71d4c89119d2e4e08b35275146d5c1',digest;(out/'output.bin').write_bytes(r.stdout);record=dict(synthetic=valid,invalid=len(qs)-valid,statusCounts=counts,aliasRejects=9,missingRuntimeTransactional=True,partialRejected=True,editionProfiles=2,bigEndian=a.m68k_linux,sha256=digest,native=False);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
