#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic byte-level removal controls, independent of owner media/receipts."""
from pathlib import Path
import argparse,struct,subprocess,os,json,hashlib
from test_original_world import valid_fixture
from test_original_game import decode
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','vehicle_removal.cpp','vehicle_removal_probe.cpp']
def sh(b,o):return struct.unpack_from('<h',b,o)[0]
def put(b,o,v):struct.pack_into('<h',b,o,v)
def cases():
 for version in range(7,13):
  for count in (1,2,9,2048):
   if count==2048 and version not in (7,12):continue
   original=valid_fixture(version,count,2);info,o,sp=decode(original);w=int(info['width']);h=int(info['height']);stride=len(sp['factions'])//8;modern=version>=11;active=0x858 if modern else 0x854;combat=0x1058 if modern else 0x1054
   positions=[(x,y)for y in range(h)for x in range(w)if not(x+y)&1]
   for victim in (range(count)if count<2048 else(0,1024,2047)):
    for var in (range(16)if count<2048 else(8,)):
     data=bytearray(original);a=bytearray(sp['vehicles']);t=bytearray(sp['tiles']);p=bytearray(sp['prototypes']);f=bytearray(sp['factions'])
     for j in range(count):
      x,y=positions[0 if var>=8 else j];put(a,j*52,x);put(a,j*52+2,y);put(a,j*52+48,j+1 if var>=8 and j+1<count else -1);put(a,j*52+50,j-1 if var>=8 else -1);a[j*52+17]=1 if(j+var)%3 else 2;put(a,j*52+20,[-1,0,victim,count-1][(j+var)%4])
     for j in range(len(t)//44):struct.pack_into('<I',t,j*44+8,struct.unpack_from('<I',t,j*44+8)[0]&~2)
     for j in range(count):
      x,y=sh(a,j*52),sh(a,j*52+2);at=((y*w+x)//2)*44+8;struct.pack_into('<I',t,at,struct.unpack_from('<I',t,at)[0]|2)
     uid=sh(a,victim*52+10);fac=a[victim*52+14];p[uid*52+42]=[0,7,8,12,255,3,6,9][var%8];f[fac*stride+active+uid]=[0,1,255,3][var%4];struct.pack_into('<i',f,fac*stride+combat,[-2147483648,-1,0,2147483647][var%4])
     for name,b in [('vehicles',a),('tiles',t),('prototypes',p),('factions',f)]:data[o[name]:o[name]+len(b)]=b
     x,y=sh(a,victim*52),sh(a,victim*52+2);nxt,prev=sh(a,victim*52+48),sh(a,victim*52+50)
     if prev>=0:put(a,prev*52+48,nxt)
     if nxt>=0:put(a,nxt*52+50,prev)
     elif prev<0:
      at=((y*w+x)//2)*44+8;struct.pack_into('<I',t,at,struct.unpack_from('<I',t,at)[0]&~2)
     for k in (0,2,48,50):put(a,victim*52+k,-1)
     a[victim*52:(count-1)*52]=a[(victim+1)*52:]
     for j in range(count-1):
      for off in (48,50):
       v=sh(a,j*52+off)
       if v>victim:put(a,j*52+off,v-1)
      v=sh(a,j*52+20)
      if a[j*52+17]==1:
       if v==victim:a[j*52+17]=0
       elif v>victim:put(a,j*52+20,v-1)
     f[fac*stride+active+uid]=(f[fac*stride+active+uid]-1)&255
     if p[uid*52+42]<8:
      at=fac*stride+combat;struct.pack_into('<I',f,at,(struct.unpack_from('<I',f,at)[0]-1)&0xffffffff)
     refs=[-1,0,victim,count-1,count,count+1,1,max(0,victim-1)];r=[refs[(var+j)%8]for j in range(5)];r[:4]=[-1 if v==victim else v-1 if v>victim else v for v in r[:4]];r[4]-=int(r[4]!=0 and r[4]>=victim)
     expected=bytearray(struct.pack('<III',0,count-1,12))+a+t
     for j in range(8):expected+=f[j*stride+active:j*stride+active+512]+f[j*stride+combat:j*stride+combat+4]
     expected+=struct.pack('<8i',*r,victim,x,y)
     yield bytes(data),victim,var,bytes(expected)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rows=list(cases());expected=b''.join(x[3]for x in rows);(out/'inputs.bin').write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<III',len(data),victim,var)+data for data,victim,var,_ in rows));(out/'expected.bin').write_bytes(expected)
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all']);exe=out/'probe';subprocess.run([args.cc,*flags,*[str(src/x)for x in SOURCES],'-o',str(exe)],check=True)
 result=subprocess.run((['qemu-m68k','-cpu','m68040']if args.m68k_linux else[])+[str(exe),str(out/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(out/'actual.bin').write_bytes(result.stdout);(out/'stderr').write_bytes(result.stderr);assert result.returncode==0,(result.returncode,len(result.stdout),result.stderr);assert result.stdout==expected,(len(result.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(result.stdout,expected))if x!=y),None));receipt={'cases':len(rows),'rejections':len(rows)*12,'versions':list(range(7,13)),'physicalActorCounts':[1,2,9,2048],'outputSHA256':hashlib.sha256(result.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
