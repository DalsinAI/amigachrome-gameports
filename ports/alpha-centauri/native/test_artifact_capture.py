#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent wire model: bounded ground artifact transfer, no fake callback."""
from pathlib import Path
import argparse,struct,subprocess,os,json,hashlib
from test_world_casualty import decode,u32,set32
from test_vehicle_removal import cases as removal_cases,sh,put
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','vehicle_stack.cpp','vehicle_removal.cpp','world_vehicle_removal.cpp','artifact_capture.cpp','artifact_capture_probe.cpp']
def rows():
 for data,id,var,_ in removal_cases():
  info,o,sp=decode(data);n=int(info['vehicles'])
  if n<2:continue
  data=bytearray(data);a=bytearray(sp['vehicles']);t=bytearray(sp['tiles']);p=bytearray(sp['prototypes']);f=bytearray(len(sp['factions']));stride=len(f)//8;active=0x858 if stride==8396 else 0x854;lost=0xc58 if stride==8396 else 0xc54;combat=0x1058 if stride==8396 else 0x1054;counter=0x2084 if stride==8396 else 0x2074
  uid=sh(a,id*52+10);other=(id+1)%n;otheruid=sh(a,other*52+10);oldfac=var%8;newfac=oldfac if var%3==0 and oldfac else 1+var%7;a[id*52+14]=oldfac;a[other*52+14]=newfac
  p[uid*52+32:uid*52+36]=bytes(4);p[uid*52+36:uid*52+43]=bytes([0,0,0,1,0,0,12]);p[otheruid*52+36]=var%2
  a[id*52+16]=255;a[id*52+17]=1;a[id*52+36]=var%7;a[id*52+40]=[0,255][var%2];put(a,id*52+46,-1);set32(a,id*52+4,0x83804200);struct.pack_into('<H',a,id*52+8,32 if var&8 else 0)
  for j in range(8):
   f[j*stride+active+uid]=[0,1,255,3][(var+j)%4];struct.pack_into('<H',f,j*stride+lost+uid*2,65535 if j==oldfac else j);set32(f,j*stride+combat,0x80000000+j);set32(f,j*stride+counter,0x7fffffff-j);set32(f,j*stride+0x220,0x12345600+j)
  for j in range(len(t)//44):t[j*44]=(t[j*44]&31)|96
  for name,b in [('vehicles',a),('tiles',t),('prototypes',p),('factions',f),('faction_definitions',bytes(len(sp['faction_definitions'])))]:data[o[name]:o[name]+len(b)]=b
  pref=o['preferences'] if 'preferences'in o else o['legacy'];set32(data,pref,0) # actual preference: capture still preempts interlude
  yield bytes(data),id,var

def model(data,id,var):
 info,o,sp=decode(data);n=int(info['vehicles']);w=int(info['width']);a=bytearray(sp['vehicles']);t=bytearray(sp['tiles']);f=bytearray(sp['factions']);stride=len(f)//8;active=0x858 if stride==8396 else 0x854;lost=0xc58 if stride==8396 else 0xc54;combat=0x1058 if stride==8396 else 0x1054;counter=0x2084 if stride==8396 else 0x2074
 set32(a,id*52+4,u32(a,id*52+4)^0x400);struct.pack_into('<H',a,id*52+8,struct.unpack_from('<H',a,id*52+8)[0]^8);t[1]^=1
 before=bytes(a[id*52:(id+1)*52]);uid=sh(before,10);oldfac=before[14];other=(id+1)%n;newfac=a[other*52+14];x,y=sh(before,0),sh(before,2);nxt,prev=sh(before,48),sh(before,50)
 at=oldfac*stride+lost+uid*2;struct.pack_into('<H',f,at,(struct.unpack_from('<H',f,at)[0]+1)&65535)
 if prev>=0:put(a,prev*52+48,nxt)
 if nxt>=0:put(a,nxt*52+50,prev)
 elif prev<0:set32(t,((y*w+x)//2)*44+8,u32(t,((y*w+x)//2)*44+8)&~2)
 for off in (0,2,48,50):put(a,id*52+off,-1)
 a[id*52+14]=newfac;a[id*52+16]=0;a[id*52+17]=0;set32(a,id*52+4,u32(a,id*52+4)&0xfcffbdff)
 at=oldfac*stride+active+uid;f[at]=(f[at]-1)&255;at=newfac*stride+active+uid;f[at]=(f[at]+1)&255
 dx,dy=sh(a,other*52),sh(a,other*52+2);top=next(j for j in range(n)if j!=id and sh(a,j*52)==dx and sh(a,j*52+2)==dy and sh(a,j*52+50)==-1)
 put(a,id*52,dx);put(a,id*52+2,dy);put(a,id*52+48,top);put(a,id*52+50,-1);put(a,top*52+50,id)
 at=((dy*w+dx)//2)*44+8;set32(t,at,u32(t,at)|0x100002)
 speed=(1+var%4+(before[36]==6))*(1+var%3);a[id*52+40]=speed
 out=bytearray(struct.pack('<III',n,23,7))+a+t
 for j in range(8):out+=f[j*stride+active:j*stride+active+512]+f[j*stride+lost:j*stride+lost+1024]+f[j*stride+combat:j*stride+combat+4]+f[j*stride+counter:j*stride+counter+4]+f[j*stride+0x220:j*stride+0x220+4]
 out+=struct.pack('<8i',0,id,n,-1,id,-1,x,y)+before
 out+=struct.pack('<6I',bool(var&1),0x7fffffff,bool(var&2),(0x7fffffff+bool(var&2))&0xffffffff,bool(var&4),0xfffffffb)
 out+=struct.pack('<5I',id,other,oldfac,newfac,speed)
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--reuse',type=Path);args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 if args.reuse:
  for name in ('inputs.bin','expected.bin'):(out/name).symlink_to(args.reuse.resolve()/name)
  total=struct.unpack('<I',(out/'inputs.bin').read_bytes()[:4])[0];expected=(out/'expected.bin').read_bytes()
 else:
  cs=list(rows());total=len(cs);expected=b''.join(model(*c)for c in cs);(out/'inputs.bin').write_bytes(struct.pack('<I',total)+b''.join(struct.pack('<III',len(data),id,var)+data for data,id,var in cs));(out/'expected.bin').write_bytes(expected)
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=out/'probe';subprocess.run([args.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if args.m68k_linux else[])+[str(exe),str(out/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(out/'actual.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,len(r.stdout),r.stderr[:1500]);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));receipt={'cases':total,'rejectionGuards':23*total,'heldRouteChecks':7*total,'outputSHA256':hashlib.sha256(expected).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
