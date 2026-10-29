# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
from test_world_basic_combat import controls as inherited_controls
from test_original_game import decode
from test_units import sections,material
from test_sensor_combat import model
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','sensor_combat.cpp','world_sensor_combat.cpp','world_sensor_combat_probe.cpp']
OFFSETS=list(zip([0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0],[0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4]))
def controls():
 for k,(name,v,text,data)in enumerate(inherited_controls()):
  b=bytearray(data);info,o,sp=decode(b);bs=308 if v=='expansion'else 304;fs=0x20cc if v=='expansion'else 0x20bc;pf=o['preferences']if v=='expansion'else o['legacy']
  struct.pack_into('<i',b,pf+12,0x80 if k%3==0 else 0);struct.pack_into('<i',b,o['metadata']+28,k%2)
  for i in range(len(sp['tiles'])//44):
   at=o['tiles']+i*44;b[at]=0x40 if (i+k)%5==0 else 0x60;b[at+7]=[-1,0,5,7][k%4]&255;items=struct.unpack_from('<I',b,at+8)[0]&0x7fffffff
   if k%4 and (i+k)%11==0:items|=0x80000000
   struct.pack_into('<I',b,at+8,items)
  for f in range(8):
   for owner in range(8):struct.pack_into('<I',b,o['factions']+f*fs+0x18+owner*4,0x08000008 if k%2 else 0)
  for i in range(int(info['bases'])):
   at=o['bases']+i*bs;b[at+0x8c+5]=(b[at+0x8c+5]&~2)|(2 if k%4>=2 and (i+k)%2==0 else 0);b[at+4]=[5,7][(i+k)%2]
  yield name,v,text,bytes(b)
 for name,v,text,data in [next(x for x in inherited_controls()if x[1]=='base'),next(x for x in inherited_controls()if x[1]=='expansion')]:
  for last in[False,True]:
   b=bytearray(data);info,o,sp=decode(b);bs=308 if v=='expansion'else 304
   for i in range(len(sp['tiles'])//44):
    at=o['tiles']+i*44;b[at]=0x60;b[at+7]=5;struct.pack_into('<I',b,at+8,struct.unpack_from('<I',b,at+8)[0]&0x7fffffff)
   for i in range(int(info['vehicles'])):struct.pack_into('<hh',b,o['vehicles']+52*i,2,2);b[o['vehicles']+52*i+14]=5
   for i in range(int(info['bases'])):
    at=o['bases']+i*bs;struct.pack_into('<hhB',b,at,2,2,5);b[at+0x8c+5]=(b[at+0x8c+5]&~2)|(2 if (i==int(info['bases'])-1)==last else 0)
   yield v+'-duplicate-tie-'+str(int(last)),v,text,bytes(b)
 for name,v,text,data in [next(x for x in inherited_controls()if x[1]=='base'),next(x for x in inherited_controls()if x[1]=='expansion')]:
  for kind in['no-base-sea-array','invalid-sensor-owner']:
   b=bytearray(data);info,o,sp=decode(b);bs=308 if v=='expansion'else 304
   for i in range(len(sp['tiles'])//44):
    at=o['tiles']+i*44;b[at]=0x60;b[at+7]=8 if kind=='invalid-sensor-owner'else 5;struct.pack_into('<I',b,at+8,0)
   at=o['tiles']+(10 if kind=='no-base-sea-array'else 9)*44;b[at]=0x40 if kind=='no-base-sea-array'else 0x60;struct.pack_into('<I',b,at+8,0x80000000)
   for i in range(int(info['vehicles'])):
    at=o['vehicles']+i*52;struct.pack_into('<hh',b,at,2,2);b[at+14]=5;struct.pack_into('<h',b,at+46,-1)
   if kind=='no-base-sea-array':
    for key in ['legacy','preferences']:
     if key in o:struct.pack_into('<i',b,o[key]+0x3c,0)
    del b[o['bases']:o['bases']+int(info['bases'])*bs]
   yield v+'-'+kind,v,text,bytes(b)
def coverage(data,actor):
 info,o,sp=decode(data);width=int(info['width']);height=int(info['height']);flat=bool(struct.unpack_from('<i',sp['metadata'],28)[0]);v=int(info['version'])>=11;bs=308 if v else 304;fs=0x20cc if v else 0x20bc;pf=sp['preferences']if v else sp['legacy'];a=sp['vehicles'][actor*52:(actor+1)*52];x,y=struct.unpack_from('<hh',a);f=a[14];tiles=sp['tiles'];bases=[sp['bases'][i*bs:(i+1)*bs]for i in range(int(info['bases']))]
 def tile(x,y):return tiles[((y*width+x)//2)*44:((y*width+x)//2+1)*44]
 def near(x,y):
  best=9999;at=-1
  for i,b in enumerate(bases):
   bx,by=struct.unpack_from('<hh',b);dx=abs(x-bx);dy=abs(y-by)
   if not flat and dx>width//2:dx=width-dx
   high,low=max(dx,dy),min(dx,dy);distance=high-(((dx+dy)//2-low+1)//2)
   if distance<=best:at=i;best=distance
  return at
 def status(x,y,t):
  if struct.unpack_from('<I',t,8)[0]&0x80000000:return 1
  if not v:return 0
  i=near(x,y)
  if i<0:return 0
  bx,by=struct.unpack_from('<hh',bases[i]);dx=abs(x-bx);dy=abs(y-by)
  if not flat and dx>width//2:dx=width-dx
  return 2 if dx in[0,2]and dy in[0,2]and bases[i][0x8c+5]&2 else 0
 if not f or tile(x,y)[0]&224<96:return 0
 result=0
 for dx,dy in OFFSETS:
  xx,yy=x+dx,y+dy
  if not flat:
   if xx<0:xx+=width
   elif xx>=width:xx-=width
  if not(0<=xx<width and 0<=yy<height):continue
  t=tile(xx,yy);s=status(xx,yy,t)
  if not s:continue
  if t[0]&224<96:
   i=near(xx,yy)
   if i<0:return None
   eligible=bases[i][4]==f
  else:
   owner=t[7]if t[7]<128 else t[7]-256
   if owner>7:return None
   treaty=struct.unpack_from('<I',sp['factions'],f*fs+0x18+4*max(0,owner))[0];rules=struct.unpack_from('<I',pf,12)[0]
   territory=-1 if owner<1 or owner!=f and not rules&128 and treaty&0x08000008!=0x08000008 else owner;eligible=territory<0 or territory==f
  if eligible:result|=1|(2 if s>1 else 0)
 return result
def expected(text,data):
 info,o,sp=decode(data);v=int(info['version'])>=11;rows=[];t=sections(text);from test_world_base_combat import RULE_ROWS
 percent=int(t['RULES'][RULE_ROWS['combat_pct_def_range_sensor']][0])
 for i in range(int(info['vehicles'])):
  c=coverage(data,i)
  for n in[720,-6789,0,2147483647,-2147483648]:
   if c is None:rows.append([1,0,1,0,0]);continue
   m=model([int(v),n,c,percent]);rows.append([0,c,*m])
 return b''.join(struct.pack('<5I',*(x&0xffffffff for x in r))for r in rows)+struct.pack('<I',20 if int(info['bases'])else 18),len(rows)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for i,(name,variant,text,data) in enumerate(controls()):
  rp=out/f'{i}.txt';gp=out/f'{i}.SC';rp.write_bytes(text);gp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(gp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);wanted,nq=expected(text,data);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None));assert gp.read_bytes()==data and rp.read_bytes()==text;h.update(r.stdout);records.append({'name':name,'queries':nq,'guards':struct.unpack('<I',r.stdout[-4:])[0]})
 receipt={'worlds':len(records),'queries':sum(r['queries'] for r in records),'guards':sum(r['guards']for r in records),'outputSHA256':h.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'records':records,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
