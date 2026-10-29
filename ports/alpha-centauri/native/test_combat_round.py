# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,random,subprocess,os,hashlib,json,argparse
parser=argparse.ArgumentParser()
parser.add_argument('--out',type=Path,required=True)
parser.add_argument('--cc',default='g++')
parser.add_argument('--m68k-linux',action='store_true')
args=parser.parse_args();D=args.out.resolve();D.mkdir(parents=True,exist_ok=True)
SRC=Path(__file__).resolve().parent
probe=D/'probe'
flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']
flags+=['-m68040','-static'] if args.m68k_linux else ['-fsanitize=address,undefined']
subprocess.run([args.cc,*flags,str(SRC/'combat_round.cpp'),str(SRC/'combat_round_probe.cpp'),'-o',str(probe)],check=True)
rng=random.Random(19661026)
rows=[]
for _ in range(18000):
 rows.append([rng.choice([0,1,2,3,32767,32768,2147483647]),rng.choice([0,1,2,3,32767,32768,2147483647]),rng.randrange(2),rng.choice([0,12,255]),rng.choice([0,1,2,4,99,100,101,255]),rng.choice([0,1,9,10,39,254,255]),rng.choice([0,12,255]),rng.choice([0,1,2,4,99,100,101,255]),rng.choice([0,1,9,10,39,254,255]),rng.getrandbits(32)])
for field in [0,1,3,4,5,6,7,8]:
 for value in [-1,256,2147483647]:
  a=[3,2,0,0,1,0,0,1,0,1];a[field]=value;rows.append(a)
def health(p,r,d):
 power=1 if p==12 else min(100,max(1,r))*10
 return power,max(0,power-d)
def expected(a):
 off,de,psi,ap,ar,ad,bp,br,bd,seed=a
 invalid=off<0 or de<0 or any(x<0 or x>255 for x in a[3:9])
 if invalid:return [1]+[0]*13+[5]
 pa,ha=health(ap,ar,ad);pb,hb=health(bp,br,bd);iah,ibh=ha,hb;n=0;ra=rb=hit=0
 if ha and hb:
  if off>1:seed=(seed*214013+2531011)&0xffffffff;ra=((seed>>16)&32767)%off;n+=1
  if de>1:seed=(seed*214013+2531011)&0xffffffff;rb=((seed>>16)&32767)%de;n+=1
  if ra<=rb:hit=1;ad=(ad+(ar if psi else 1))&255
  else:hit=2;bd=(bd+(br if psi else 1))&255
  _,ha=health(ap,ar,ad);_,hb=health(bp,br,bd)
 return [0,seed,n,ra,rb,ad,bd,ha,hb,hit,pa,pb,iah,ibh,4]
data=b''.join(struct.pack('<10I',*(x&0xffffffff for x in a)) for a in rows)
want=b''.join(struct.pack('<15I',*expected(a)) for a in rows)
(D/'inputs.bin').write_bytes(data);(D/'expected.bin').write_bytes(want)
for cmd in [(['qemu-m68k','-cpu','m68040'] if args.m68k_linux else [])+[str(probe)]]:
 r=subprocess.run(cmd,input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 (D/(Path(cmd[-1]).name+'.stderr')).write_bytes(r.stderr)
 assert r.returncode==0,(cmd,r.returncode,r.stderr[:1000])
 assert r.stdout==want,next((i,rows[i],struct.unpack('<15I',r.stdout[i*60:(i+1)*60]),expected(rows[i]))for i in range(len(rows))if r.stdout[i*60:(i+1)*60]!=want[i*60:(i+1)*60])
 (D/(Path(cmd[-1]).name+'.output')).write_bytes(r.stdout)
 receipt={'command':cmd,'cases':len(rows),'accepted':sum(expected(a)[0]==0 for a in rows),'outputSHA256':hashlib.sha256(r.stdout).hexdigest()}
 (D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
