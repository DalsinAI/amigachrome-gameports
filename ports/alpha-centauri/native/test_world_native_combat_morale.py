# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
import test_ready_search as ready
from test_original_game import decode
from test_native_combat_morale import model
HERE=Path(__file__).resolve().parent
SOURCES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','native_combat_morale.cpp','world_native_combat_morale.cpp','world_native_combat_morale_probe.cpp']
def fixture(version,case):
 _,data,_=ready.fixture(version,case);b=bytearray(data);info,o,sp=decode(b);pf=o['legacy'] if version<11 else o['preferences'];struct.pack_into('<i',b,pf+0x44,[-1,44,45,89,90,169,170,249,250,329,330,1000][case%12]);fs=0x20cc if version>=11 else 0x20bc
 for f in range(8):struct.pack_into('<i',b,o['factions']+fs*f+0x1d8,[-1,0,1][(f+case)%3])
 for i in range(int(info['vehicles'])):
  p=o['vehicles']+i*52;struct.pack_into('<h',b,p+10,[0,10,19,511][(case+i)%4]);struct.pack_into('<I',b,p+4,256 if (case+i)%2 else 0);struct.pack_into('<H',b,p+8,[0,8,16,65535][(case+i)%4]);b[p+14]=(case+i)%8
 width=int(info['width']);height=int(info['height']);tiles=(width//2)*height
 for i in range(tiles):
  p=o['tiles']+i*44;b[p]=((case+i)%8)*32;struct.pack_into('<I',b,p+8,0x20 if (case+i)%3 else 0)
 if version==12:
  for f in range(8):struct.pack_into('<i',b,o['tectonic']+4*f,0 if (case+f)%2 else -1)
 return bytes(b)
def expected(data):
 info,o,sp=decode(data);version=int(info['version']);pf=sp['legacy'] if version<11 else sp['preferences'];turn=struct.unpack_from('<i',pf,0x44)[0];fs=0x20cc if version>=11 else 0x20bc;width=int(info['width']);height=int(info['height']);flat=bool(struct.unpack_from('<I',sp['metadata'],28)[0]);rows=[]
 for i in range(int(info['vehicles'])):
  a=sp['vehicles'][52*i:52*(i+1)];uid=struct.unpack_from('<h',a,10)[0];x,y=struct.unpack_from('<hh',a);count=0
  for dx,dy in [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]:
   tx=x+dx;ty=y+dy
   if not flat:tx%=width
   if not (0<=tx<width and 0<=ty<height):continue
   p=(ty*(width//2)+tx//2)*44;t=sp['tiles'][p:p+44]
   if struct.unpack_from('<I',t,8)[0]&32 and t[0]&224>=64:count+=1
  for f in range(-1,8):
   if version==11 and f>0:rows.append((1,0x12345678));continue
   atro=struct.unpack_from('<i',sp['factions'],fs*f+0x1d8)[0] if f>0 else 0;tect=struct.unpack_from('<i',sp['tectonic'],4*f)[0] if version==12 and f>0 else 0
   m=model([int(version>=11),turn,uid,f,atro,tect,struct.unpack_from('<I',a,4)[0],struct.unpack_from('<H',a,8)[0],count]);rows.append(tuple(m[:2]))
 return b''.join(struct.pack('<2I',*r) for r in rows)+struct.pack('<I',8),len(rows),sum(r[0]==0 for r in rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--private-media',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[];inputs=[(f'v{v}-{c}',fixture(v,c)) for v in range(7,13) for c in range(24)]
 if a.private_media:
  for name in ['Explore.SC','Build.SC','conquer.SC']:inputs.append((name,(a.private_media/name).read_bytes()))
 for name,data in inputs:
  result,nq,accepted=expected(data);i=len(records);sp=out/f'{i}.SC';sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(sp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==result,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,result)) if x!=y),None));assert sp.read_bytes()==data;h.update(r.stdout);records.append({'name':name,'queries':nq,'accepted':accepted})
 receipt={'worlds':len(records),'queries':sum(x['queries'] for x in records),'accepted':sum(x['accepted'] for x in records),'rejected':sum(x['queries']-x['accepted'] for x in records),'guards':8*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
