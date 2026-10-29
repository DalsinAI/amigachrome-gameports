# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,random,struct,subprocess,os,json,hashlib

def cases():
 r=random.Random(592140);rows=[];flags=[0,0x10000000,0x2000,0x100000,0x4000000,0x8000000,0xc000000,0x102000]
 for i in range(20000):
  x=r.randrange(64);y=r.randrange(32)*2+(x&1);rows.append([64,64,r.choice([0,1,17,0xffffffff,r.getrandbits(32)]),r.randrange(256),r.choice(flags),x,y,0x2000 if i%5==0 else 0,61 if i%3 else r.getrandbits(32),11 if i%3 else r.getrandbits(32)])
 for field in [0,1,5,6]:
  for bad in [0xffffffff,0x80000000,4097]:
   a=[64,64,1,96,0,2,2,0,61,11];a[field]=bad;rows.append(a)
 rows.append([64,64,1,96,0,1,2,0,61,11])
 for seed in [0,1,0xffffffff]:
  for x,y in [(0,0),(4094,4094),(1,4095),(4095,1),(4095,4095)]:rows.append([4096,4096,seed,96,0,x,y,0,0x80000000,0x7fffffff])
 return rows

def model(a):
 w,h,seed,alt,items,x,y,rules,dc,ac=a;sign=lambda n:n if n<2**31 else n-2**32;x=sign(x);y=sign(y)
 if not(2<=w<=4096 and w%2==0 and 1<=h<=4096 and 0<=x<w and 0<=y<h and(x+y)%2==0):return [1,0,3]
 if items&0x102000:return [0,0,3]
 if rules&0x2000:return[0,2 if items&0xc000000 else 0,3]
 if items&0x10000000:return[0,1,3]
 if not seed:return[0,0,3]
 avg=(x+y)//2;diff=x-avg;code=(avg%4)+4*(diff%4)
 if(alt&224)>=96 and code==(seed-5*((x+y)//8)-3*(diff//4))%16:return[0,2,3]
 trunc=lambda n:abs(n)//4*(-1 if n<0 else 1)
 return[0,int(code==(seed+8+trunc(diff)*sign(dc)+trunc(avg)*sign(ac))%32),3]

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();d=a.out.resolve();d.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti'];flags+=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all'];exe=d/'probe';subprocess.run([a.cc,*flags,str(src/'supply_pods.cpp'),str(src/'supply_pods_probe.cpp'),'-o',str(exe)],check=True)
 rows=cases();data=b''.join(struct.pack('<10I',*row)for row in rows);want=b''.join(struct.pack('<3I',*model(row))for row in rows);(d/'inputs.bin').write_bytes(data);(d/'expected.bin').write_bytes(want);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe)];r=subprocess.run(cmd,input=data,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'stderr').write_bytes(r.stderr);assert not r.returncode,(r.returncode,r.stderr);(d/'actual.bin').write_bytes(r.stdout);assert r.stdout==want,next((i,row,struct.unpack_from('<3I',r.stdout,i*12),model(row))for i,row in enumerate(rows)if r.stdout[i*12:(i+1)*12]!=want[i*12:(i+1)*12]);receipt={'cases':len(rows),'accepted':sum(not model(a)[0]for a in rows),'outputSHA256':hashlib.sha256(want).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
