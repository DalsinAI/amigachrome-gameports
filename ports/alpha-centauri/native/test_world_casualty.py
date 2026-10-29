#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent wire model for bounded casualty/deletion, not legal battle."""
from pathlib import Path
import argparse,struct,subprocess,os,json,hashlib
from test_vehicle_removal import cases as removal_cases,sh,put
import test_vehicle_removal
def decode(data):
 assert data[:12]==b'TERRAN\0\x1a'+struct.pack('<I',0x56)
 pos=12;spans={};offsets={}
 def take(name,n):
  nonlocal pos
  assert n>=0 and pos+n<=len(data),(name,pos,n,len(data))
  offsets[name]=pos;spans[name]=data[pos:pos+n];pos+=n;return spans[name]
 legacy=take('legacy',796);marker=struct.unpack_from('<i',legacy,0x304)[0];expanded=marker>=11
 p=take('preferences',920) if expanded else legacy
 def pref(off):return struct.unpack_from('<i',p,off if expanded or off<0x184 else off-0x7c)[0]
 version=pref(0x380);vehicles=pref(0x38);bases=pref(0x3c)
 player=struct.unpack('<i',take('player',4))[0]
 take('factions',0x10660 if version>=11 else 0x105e0);take('faction_definitions',0x2ce0);take('rankings',0x20)
 m=take('metadata',0xaa4);w,h=struct.unpack_from('<ii',m);nt=w//2*h;ac=((h+4)//5)*((((w+4)//5)+1)//2)
 take('tiles',nt*44);take('abstract',ac);take('faction_turn_might',0x3e80)
 replay=struct.unpack('<i',take('replay_size',4))[0];take('replay',replay);take('continents',0xe00);take('prototypes',0x6800);take('bunglist',0x1000)
 take('vehicles',vehicles*52);take('bases',bases*(308 if version>=11 else 304))
 if pref(0x30)&0x10:take('scenario',0x100)
 else:spans['scenario']=b''
 take('scenario_name',0x100);take('objectives',0x98);take('monuments',0x27a0)
 if version>=12:take('tectonic',0x20)
 else:spans['tectonic']=b''
 ui=take('ui',244);tick=struct.unpack('<I',take('tick',4))[0]
 assert pos==len(data),(pos,len(data))
 return dict(version=str(version),vehicles=str(vehicles),width=str(w),height=str(h),turn=str(pref(0x44))),offsets,spans

# Reuse the bounded wire parser without unrelated legacy digest generation.
test_vehicle_removal.decode=decode
import test_original_world
test_original_world.decode=decode
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','vehicle_removal.cpp','world_vehicle_removal.cpp','prototype.cpp','movement.cpp','native_combat_morale.cpp','world_native_combat_morale.cpp','world_casualty.cpp','world_casualty_probe.cpp']
def u32(b,o):return struct.unpack_from('<I',b,o)[0]
def set32(b,o,n):struct.pack_into('<I',b,o,n&0xffffffff)
def rows():
 for data,id,var,_ in removal_cases():
  info,offsets,sp=decode(data);n=int(info['vehicles']);version=int(info['version']) if 'version'in info else None
  # Version is the container header, independently recovered by decode below.
  if n==2048 and var!=8:continue
  for native in (False,True) if n>1 else (False,):
   data=bytearray(data);a=bytearray(sp['vehicles']);p=bytearray(sp['prototypes']);f=bytearray(sp['factions']);stride=len(f)//8;modern=stride==0x20cc;uid=sh(a,id*52+10)
   if not native:a[id*52+14]=max(1,a[id*52+14])
   if native:
    uid=[0,10,19,511][var%4];put(a,id*52+10,uid);a[id*52+14]=0;a[((id+1)%n)*52+14]=1
   p[uid*52+42]=[0,7,8,9][var%4]
   struct.pack_into('<H',a,id*52+8,[(var%4)*8,0x20+(var%4)*8][bool(var&8)])
   set32(a,id*52+4,[0,0x100][bool(var&2)])
   fac=a[id*52+14];lost=0xc58 if modern else 0xc54;counter=0x2084 if modern else 0x2074
   struct.pack_into('<H',f,fac*stride+lost+uid*2,65535 if var&1 else 9)
   for j in range(8):set32(f,j*stride+counter,0x7fffffff if var&1 else 0xffffffff)
   for name,b in [('vehicles',a),('prototypes',p),('factions',f)]:data[offsets[name]:offsets[name]+len(b)]=b
   pref=offsets['preferences'] if 'preferences'in offsets else offsets['legacy'];struct.pack_into('<i',data,pref+0x44,[44,45,89,90,169,170,329,330][var%8])
   yield bytes(data),id,var

def model(data,id,var):
 info,o,sp=decode(data);n=int(info['vehicles']);w=int(info['width']);a=bytearray(sp['vehicles']);t=bytearray(sp['tiles']);f=bytearray(sp['factions']);p=sp['prototypes'];stride=len(f)//8;modern=stride==0x20cc;active=0x858 if modern else 0x854;lost=0xc58 if modern else 0xc54;combat=0x1058 if modern else 0x1054;counter=0x2084 if modern else 0x2074
 set32(a,id*52+4,u32(a,id*52+4)^256);struct.pack_into('<H',a,id*52+8,struct.unpack_from('<H',a,id*52+8)[0]^8);set32(t,8,u32(t,8)^32)
 removed=bytes(a[id*52:(id+1)*52]);uid=sh(removed,10);fac=removed[14];x,y=sh(removed,0),sh(removed,2);nxt,prev=sh(removed,48),sh(removed,50)
 at=fac*stride+lost+uid*2;struct.pack_into('<H',f,at,(struct.unpack_from('<H',f,at)[0]+1)&65535)
 if struct.unpack_from('<H',removed,8)[0]&0x20 and var%9-1>=0:at=(var%9-1)*stride+counter;set32(f,at,u32(f,at)+1)
 reward=0
 if fac==0 and a[((id+1)%n)*52+14]!=0 and var&4:
  # CurrentTurn in synthetic fixture: decode exports this exact preference.
  turn=int(info['turn']);rank=next((i for i,b in enumerate([45,90,170,250,330])if turn<b),6)+bool(u32(removed,4)&256)+(uid==10)+((struct.unpack_from('<H',removed,8)[0]>>3)&3)
  if modern and uid==19:
   count=0
   flat=bool(u32(data,o['tiles']-0xaa4+28))
   for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
    xx,yy=x+dx,y+dy
    if not flat:xx%=w
    if 0<=xx<w and 0<=yy<int(info['height']):
     tt=((yy*w+xx)//2)*44;count+=bool(u32(t,tt+8)&32 and t[tt]&224>=64)
   rank=count-2
  reward=(min(6,max(0,rank))+1)*10
 if prev>=0:put(a,prev*52+48,nxt)
 if nxt>=0:put(a,nxt*52+50,prev)
 elif prev<0:set32(t,((y*w+x)//2)*44+8,u32(t,((y*w+x)//2)*44+8)&~2)
 for off in (0,2,48,50):put(a,id*52+off,-1)
 a[id*52:(n-1)*52]=a[(id+1)*52:]
 for j in range(n-1):
  for off in (48,50):
   if sh(a,j*52+off)>id:put(a,j*52+off,sh(a,j*52+off)-1)
  v=sh(a,j*52+20)
  if a[j*52+17]==1:
   if v==id:a[j*52+17]=0
   elif v>id:put(a,j*52+20,v-1)
 at=fac*stride+active+uid;f[at]=(f[at]-1)&255
 if p[uid*52+42]<8:at=fac*stride+combat;set32(f,at,u32(f,at)-1)
 out=bytearray(struct.pack('<II',n-1,22))+a[:(n-1)*52]+t
 for j in range(8):out+=f[j*stride+active:j*stride+active+512]+f[j*stride+lost:j*stride+lost+1024]+f[j*stride+combat:j*stride+combat+4]+f[j*stride+counter:j*stride+counter+4]+f[j*stride+0x220:j*stride+0x220+4]
 refs=[0,id,n,-1];refs=[-1 if v==id else v-1 if v>id else v for v in refs];out+=struct.pack('<8i',*refs,id-int(id!=0),id,x,y)+removed
 out+=struct.pack('<6I',bool(var&1),(0x7fffffff+bool(var&1))&0xffffffff,bool(var&2),0x80000001,bool(var&4),(0x7fffffff+reward)&0xffffffff)
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--reuse',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 if a.reuse:
  for name in ('inputs.bin','expected.bin'):(out/name).symlink_to(a.reuse.resolve()/name)
  total=struct.unpack('<I',(out/'inputs.bin').read_bytes()[:4])[0];expected=(out/'expected.bin').read_bytes()
 else:
  cs=list(rows());total=len(cs);expected=b''.join(model(*c)for c in cs);(out/'inputs.bin').write_bytes(struct.pack('<I',total)+b''.join(struct.pack('<III',len(data),id,var)+data for data,id,var in cs));(out/'expected.bin').write_bytes(expected)
 flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=out/'probe';subprocess.run([a.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(out/'inputs.bin')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(out/'actual.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,len(r.stdout),r.stderr[:1000]);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));receipt={'cases':total,'guards':22*total,'outputSHA256':hashlib.sha256(expected).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
