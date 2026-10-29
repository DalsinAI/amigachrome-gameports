# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_original_game import decode
from test_units import reference
from test_terrain_defense import model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','terrain_defense.cpp','world_terrain_defense.cpp','world_terrain_defense_probe.cpp']
def controls():
 for k,(name,v,text,data) in enumerate(inherited_controls()):
  # Explicit owned tile facts independent of saved economy, physical base records,
  # and prototype plans. Full stored owner nibble is distinct from territory byte7.
  b=bytearray(data);info,o,sp=decode(b);width=int(info['width']);n=int(info['vehicles']);bs=308 if v=='expansion' else 304;pf=o['preferences']if v=='expansion'else o['legacy']
  for tile in range(int(info['tiles'])) if 'tiles'in info else range(len(sp['tiles'])//44):
   at=o['tiles']+tile*44;b[at]=[0x40,0x60,0x80,255][(tile+k)%4];b[at+5]=[0,0x40,0x80,255][(tile+k//4)%4];b[at+2]=(b[at+2]&0xf0)|[0,7,8,15][(tile+k//8)%4];struct.pack_into('<I',b,at+8,[0,1,0x20,0x200000,0x200020,0x200021][(tile+k)%6])
  for actor in range(n):
   uid=struct.unpack_from('<h',b,o['vehicles']+actor*52+10)[0];b[o['prototypes']+uid*52+36]=(actor+k)%9
  loc=[-1,0,1][k%3];struct.pack_into('<i',b,pf+0x84+16*4,loc)
  if loc>=0:b[o['bases']+loc*bs+4]=[0,5,7][k%3]
  yield name,v,text,bytes(b)
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;tab=reference(text,'expansion'if v else'base');n=int(info['vehicles']);width=int(info['width']);pf=sp['preferences']if v else sp['legacy'];bs=308 if v else 304;loc=struct.unpack_from('<i',pf,0x84+16*4)[0];ph=sp['bases'][loc*bs+4]if loc>=0 else -1;rows=[]
 def actor(i):
  a=sp['vehicles'][i*52:(i+1)*52];uid=struct.unpack_from('<h',a,10)[0];p=sp['prototypes'][uid*52:(uid+1)*52];return a,uid,int(tab[f'weapons.{p[37]}.offense']),int(tab[f'chassis.{p[36]}.triad'])
 for i in range(n):
  a,uid,off,triad=actor(i);x,y=struct.unpack_from('<hh',a);at=((y*width+x)//2)*44;tile=sp['tiles'][at:at+44]
  for j in range(-1,n):
   if j<0:opp=[0,0,0,0,0]
   else:b,ouid,ooff,otriad=actor(j);opp=[1,ouid,ooff,otriad,b[14]]
   c=[tile[0],tile[5],struct.unpack_from('<I',tile,8)[0],tile[2]&15,a[14],uid,off,triad,*opp,ph];rows.append(model(c)[:3])
 return b''.join(struct.pack('<3I',*r)for r in rows)+struct.pack('<I',15),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':15*len(records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
