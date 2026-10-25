#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original MP file reader vs independent byte/field interpretation."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def fixture(width=8,height=6):
 count=width//2*height;ax=(width+4)//5;ay=(height+4)//5;ac=ay*((ax+1)//2)
 m=bytearray(0xaa4);struct.pack_into('<9I',m,0,width,height,0x87654321,0xfffffffe,7,count,123,0,1)
 struct.pack_into('<ii',m,36,0,0);m[44:76]=bytes([0xe9])+bytes(range(1,32))
 struct.pack_into('<3I',m,0xa24,ax,ay,ac);struct.pack_into('<7i',m,0xa30,2,1,1,-1,0,2,3)
 struct.pack_into('<2I',m,0xa9c,0xdeadbeef,0x80001000)
 tiles=bytearray()
 for i in range(count):
  tiles+=bytes([(i*37)&255,(i*13)&255,(i*3)&255,i&255,(i*11)&255,(i*29)&255,(i*7)&255,(128+i)&255])
  tiles+=struct.pack('<9I',*[((i*0x1234567+j*0x3456789)^0x80000080)&0xffffffff for j in range(9)])
 return b'TERRANMAP\0\x1a'+struct.pack('<I',5)+m+tiles+bytes((i*17+129)&255 for i in range(ac))

def fnv(data,h=2166136261):
 for b in data:h=((h^b)*16777619)&0xffffffff
 return h

def word(h,n):return fnv(struct.pack('<I',n&0xffffffff),h)
def model(data):
 m=data[15:15+0xaa4];width,height=struct.unpack_from('<ii',m);count=width//2*height
 ax=(width+4)//5;ay=(height+4)//5;ac=ay*((ax+1)//2);seed=struct.unpack_from('<I',m,8)[0]
 sea,council=struct.unpack_from('<ii',m,12);flat=int(struct.unpack_from('<I',m,28)[0]!=0);lmcount=struct.unpack_from('<I',m,32)[0]
 differs=int(struct.unpack_from('<I',m,20)[0]!=count or struct.unpack_from('<3I',m,0xa24)!=(ax,ay,ac))
 off_map=0
 for i in range(lmcount):
  x,y=struct.unpack_from('<ii',m,36+i*40);off_map+=not(0<=x<width and 0<=y<height and not((x+y)&1))
 info=2166136261
 for n in [width,height,count,ax,ay,ac,seed,sea,council,flat,lmcount,differs,off_map,*struct.unpack_from('<7i',m,0xa30)]:info=word(info,n)
 digest=2166136261;land=fungus=forests=rivers=rocky=0
 for i in range(count):
  p=15+0xaa4+i*44;head=struct.unpack_from('<7Bb',data,p);tail=struct.unpack_from('<9I',data,p+8)
  for n in [*head,*tail]:digest=word(digest,n)
  climate,_,_,_,_,val3,_,_=head;items=tail[0];is_land=climate>>5>=3
  land+=is_land;fungus+=bool(items&0x20) and climate>>5>=2;forests+=bool(items&0x200000);rivers+=bool(items&0x80);rocky+=is_land and bool(val3&0x80)
 landmarks=2166136261
 for i in range(lmcount):
  p=36+i*40;x,y=struct.unpack_from('<ii',m,p);landmarks=word(word(word(landmarks,x),y),int(0<=x<width and 0<=y<height and not((x+y)&1)));landmarks=fnv(m[p+8:p+40],landmarks)
 return dict(width=str(width),height=str(height),tiles=str(count),abstract=str(ac),seed=f'{seed:08x}',flat=str(flat),landmarks=str(lmcount),counts_differ=str(differs),off_map_landmarks=str(off_map),land=str(land),fungus=str(fungus),forests=str(forests),rivers=str(rivers),rocky=str(rocky),tile_digest=f'{digest:08x}',info_digest=f'{info:08x}',landmark_digest=f'{landmarks:08x}',metadata_digest=f'{fnv(m):08x}',abstract_digest=f'{fnv(data[15+0xaa4+count*44:]):08x}',rc='0')

def cases():
 data=fixture();result=[('valid',data,True),('minimum',fixture(2,1),True),('wide',fixture(4096,1),True),('tall',fixture(2,4096),True)]
 def mutate(name,offset,value,valid=False):
  changed=bytearray(data);struct.pack_into('<I',changed,offset,value&0xffffffff);result.append((name,bytes(changed),valid))
 for offset in [20,0xa24,0xa28,0xa2c]:mutate('ignored-count-'+str(offset),15+offset,0xffffffff,True)
 for offset in [0xa9c,0xaa0]:mutate('inert-pointer-'+str(offset),15+offset,0xffffffff,True)
 mutate('flat-nonzero',15+28,0xffffffff,True)
 for n in range(15):result.append(('short-'+str(n),data[:n],False))
 for n in [15,99,15+0xaa4-1,15+0xaa4,len(data)-1]:result.append(('truncated-'+str(n),data[:n],False))
 for i in range(11):
  changed=bytearray(data);changed[i]^=0x80;result.append(('magic-'+str(i),bytes(changed),False))
 for version in [0,4,6,0xffffffff]:mutate('version-'+str(version),11,version)
 for value in [0,1,3,4097,0x7fffffff,0xffffffff]:mutate('width-'+str(value),15,value)
 for value in [0,4097,0x7fffffff,0xffffffff]:mutate('height-'+str(value),19,value)
 for value in [65,0xffffffff]:mutate('landmark-count-'+str(value),15+32,value)
 for name,offset,value in [('landmark-xnegative',36,-1),('landmark-xedge',36,8),('landmark-ynegative',40,-1),('landmark-yedge',40,6),('landmark-parity',36,1)]:mutate(name,15+offset,value,True)
 changed=bytearray(data);struct.pack_into('<ii',changed,15,4096,4096);result.append(('maximum-truncated',bytes(changed),False))
 result += [('trailing',data+b'\0',False),('trailing-many',data+b'\0'*200,False)]
 return result

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 src=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'map-probe';command=[a.cc,*flags,str(src/'original_map.cpp'),str(src/'original_map_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 records=[]
 for i,(name,data,valid) in enumerate(cases()):
  path=out/(str(i)+'.MP');path.write_bytes(data)
  r=subprocess.run([*runner,str(exe),str(path)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  if valid:
   actual=dict(v.split('=') for v in r.stdout.split());expected=model(data);assert actual==expected,(name,actual,expected)
  records.append(dict(name=name,passed=True,valid=valid))
 (out/'receipt.json').write_text(json.dumps(dict(command=command,runner=runner,cases=records,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(f'{len(records)} original-map cases passed')
if __name__=='__main__':main()
