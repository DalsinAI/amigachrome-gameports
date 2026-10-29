# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,json,struct,subprocess,os,hashlib
from pathlib import Path
from test_retreat_init import controls as parent_controls
from test_original_game import decode
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','prototype.cpp','movement.cpp','world_zoc.cpp','world_zoc_probe.cpp']
def controls():
 for name,variant,text,data in parent_controls():
  b=bytearray(data);info,o,s=decode(b);n=int(info['vehicles']);width=int(info['width']);k=int(name.rsplit('-',1)[1]);fs=0x20cc if variant=='expansion'else 0x20bc
  for i in range(len(s['tiles'])//44):b[o['tiles']+i*44]=[32,64,96,128][(i+k)%4];b[o['tiles']+i*44+2]=(i+k)%16
  for i in range(n):
   at=o['vehicles']+i*52;b[at+14]=1+i%3;b[at+39]=[0,2,4,255][(i+k)%4];struct.pack_into('<H',b,at+8,0x400 if(i+k)%2 else 0)
  for f in range(8):
   for other in range(8):struct.pack_into('<I',b,o['factions']+f*fs+0x18+other*4,1 if(f+other+k)%4==0 else 0)
  yield name,bytes(b)
def queries(data):
 info,_,s=decode(data);w=int(info['width']);h=int(info['height']);vs=list(struct.iter_unpack('<52s',s['vehicles']));actors=[x[0]for x in vs];n=len(actors);fs=0x20cc if int(info['version'])>=11 else 0x20bc
 # Independent validation of the intentional malformed/disconnected fixture.
 invalid=False
 coords={}
 for i,a in enumerate(actors):coords.setdefault(struct.unpack_from('<hh',a),[]).append(i)
 for xy,ids in coords.items():
  heads=[q for q in ids if struct.unpack_from('<h',actors[q],50)[0]<0]
  if len(heads)!=1:invalid=True;continue
  walk=[];at=heads[0]
  while at>=0:
   if at in walk or at not in ids:invalid=True;break
   walk.append(at);nxt=struct.unpack_from('<h',actors[at],48)[0]
   if nxt>=0 and (nxt>=n or struct.unpack_from('<h',actors[nxt],50)[0]!=at):invalid=True;break
   at=nxt
  if set(walk)!=set(ids):invalid=True
 flat=struct.unpack_from('<I',s['metadata'],28)[0]!=0
 for net in range(2):
  for move in range(2):
   for fac in range(8):
    for y in range(h):
     for x in range(y&1,w,2):
      result=0;tile=s['tiles'][((y*w+x)//2)*44:((y*w+x)//2+1)*44]
      if not invalid and not(move and struct.unpack_from('<I',tile,8)[0]&1 and(tile[2]&15)<8):
       for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
        xx=x+dx;yy=y+dy
        if not flat:xx=xx+w if xx<0 else xx-w if xx>=w else xx
        if not(0<=xx<w and 0<=yy<h):continue
        t=s['tiles'][((yy*w+xx)//2)*44:((yy*w+xx)//2+1)*44];owner=t[2]&15
        if not(struct.unpack_from('<I',t,8)[0]&2)or owner>=8 or owner==fac or ((t[0]&224)<96)!=((tile[0]&224)<96)or(struct.unpack_from('<I',s['factions'],fac*fs+0x18+owner*4)[0]&1):continue
        enemy=any(a[14]!=fac and(a[39]&(1<<fac)or(not net and not(struct.unpack_from('<H',a,8)[0]&0x400)))for a in actors if struct.unpack_from('<hh',a)==(xx,yy))
        if enemy:result=owner+1;break
      yield (x,y,fac,net,move),(1,0)if invalid else(0,result)
def expected(data):return b''.join(struct.pack('<2I',*v)for q,v in queries(data))+struct.pack('<I',11)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();d=a.out.resolve();d.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti'];flags+=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all'];exe=d/'probe';subprocess.run([a.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True);digest=hashlib.sha256();n=0
 for name,data in controls():
  folder=d/name;folder.mkdir();(folder/'world.sc').write_bytes(data);want=expected(data);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(folder/'world.sc')];r=subprocess.run(cmd,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(folder/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr);(folder/'actual.bin').write_bytes(r.stdout);(folder/'expected.bin').write_bytes(want);assert r.stdout==want,(name,[(i,r.stdout[i:i+8],want[i:i+8])for i in range(0,len(want)-4,8)if r.stdout[i:i+8]!=want[i:i+8]][:4]);digest.update(r.stdout);n+=(len(want)-4)//8
 receipt={'worlds':32,'queries':n,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest()};(d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
