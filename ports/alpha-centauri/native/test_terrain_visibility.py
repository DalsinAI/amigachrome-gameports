# SPDX-License-Identifier: GPL-3.0-or-later
"""Real owned stacks/bases and explicit render-time visibility inputs."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_terrain_observation import model as prelude
DX=[0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0]
DY=[0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4]

def fixture(profile):
 b=bytearray(valid_fixture());_,o,_=decode(b)
 for f in range(8):
  q=o['factions']+f*0x20bc;struct.pack_into('<I',b,q,0);struct.pack_into('<i',b,q+0x14,f&1)
  for other in range(8):struct.pack_into('<I',b,q+0x18+other*4,int((f+other+profile)%3==0))
  p=o['faction_definitions']+f*0x59c;struct.pack_into('<ii',b,p+0x508,0,0)
 for i in range(24):
  p=o['tiles']+i*44;b[p]=((i+profile)%5)<<5|((i%4)<<3);b[p+2]=15;b[p+4]=(i*37)%256;b[p+5]=(i%4)<<6;b[p+7]=1+i%7;struct.pack_into('<9I',b,p+8,0,(i*0x1234)|0x200000,*[0x100+j*16+i for j in range(7)])
 for i in range(3):
  p=o['vehicles']+i*52;b[p+14]=i+1;t=o['tiles']+i*44;b[t+2]=i+1;struct.pack_into('<I',b,t+8,2)
  proto=o['prototypes']+(320+i)*52;struct.pack_into('<I',b,proto+32,2 if profile in [1,2,3]else 0);b[proto+36]=profile%3
 for i in range(2):
  p=o['bases']+i*304;b[p+4]=i+1;t=o['tiles']+(8+i)*44;b[t+2]=i+1;struct.pack_into('<I',b,t+8,1)
  for n in range(8):b[p+0x8c+n]=0
 if profile==4:struct.pack_into('<I',b,o['faction_definitions']+5*0x59c+0x508,0x80)
 if profile in [5,8,9,10]:
  p=o['faction_definitions']+5*0x59c;struct.pack_into('<i',b,p+0x50c,1);struct.pack_into('<i',b,p+0x510,15);struct.pack_into('<i',b,p+0x530,1)
 if profile in [8,9,10]:b[o['factions']-4-0x31c+0x1e0-124]=0x20 if profile in [9,10]else 0
 if profile==6:struct.pack_into('<I',b,o['tiles']+15*44+8,0x80000000)
 if profile==7:b[o['bases']+0x8c+5]|=2
 return b

def model(w,o,q,profile):
 x,y,v,z,cur,draw,rules,prefs,state,edition,mx,my,mid,exp,negative,sup,source_flags=q;old=prelude(w,q[:10],o)
 if old[0]==0:return old
 if old[0]==4:return [2]+[0]*8
 if old[0]==2:return [5]+[0]*8
 if source_flags&1:return [8]+[0]*8
 if mx< -1 or my< -1 or ((mx<0)!=(my<0)) or (mx>=0 and(mx>=8 or my>=6 or (mx+my)&1 or mid<0 or mid>=3)):return [2]+[0]*8
 tiles=[w[o['tiles']+i*44:o['tiles']+(i+1)*44]for i in range(24)];vehicles=[w[o['vehicles']+i*52:o['vehicles']+(i+1)*52]for i in range(3)];bases=[w[o['bases']+i*304:o['bases']+(i+1)*304]for i in range(2)]
 def at(a,b):
  # Fixture is cylindrical, exactly one source wrap.
  if a<0:a+=8
  elif a>=8:a-=8
  return(a,b,tiles[(b*8+a)//2])if 0<=a<8 and 0<=b<6 else None
 def owner(t,mask):return(t[2]&15)if struct.unpack_from('<I',t,8)[0]&mask and(t[2]&15)<8 else-1
 def ids(a,b):return[i for i,t in enumerate(vehicles)if struct.unpack_from('<hh',t)==(a,b)]
 def pact(a,b):return struct.unpack_from('<I',w,o['factions']+a*0x20bc+(0x14 if b<0 else 0x18+b*4))[0]&1
 def radar(id):
  u=struct.unpack_from('<h',vehicles[id],10)[0];p=w[o['prototypes']+u*52:o['prototypes']+(u+1)*52];flag=struct.unpack_from('<I',p,32)[0]&2
  if edition and u>=64:
   f=o['faction_definitions']+(u//64)*0x59c;flag=flag or struct.unpack_from('<I',w,f+0x508)[0]&0x80 or(struct.unpack_from('<i',w,f+0x50c)[0]>0 and struct.unpack_from('<i',w,f+0x510)[0]==15 and struct.unpack_from('<i',w,f+0x530)[0]==1 and(profile not in [8,9,10]or(profile==9 and w[o['factions']-4-0x31c+0x1e0-124]&0x20)))
  triad=p[36]%3;return bool(vehicles[id][14]==v and flag and(triad==2 or (tiles[(y*8+x)//2][0]>>5<3)==(triad==1)))
 def sensor(a,b,t):
  if struct.unpack_from('<I',t,8)[0]&0x80000000:return True
  if not edition:return False
  near=-1;best=9999
  for i,u in enumerate(bases):
   xx,yy=struct.unpack_from('<hh',u);dx=abs(a-xx);dx=8-dx if dx>4 else dx;dy=abs(b-yy);dist=max(dx,dy)-(((dx+dy)//2-min(dx,dy)+1)//2)
   if dist<=best:near=i;best=dist
  u=bases[near];xx,yy=struct.unpack_from('<hh',u);dx=abs(a-xx);dx=8-dx if dx>4 else dx
  return dx in [0,2]and abs(b-yy)in[0,2]and bool(u[0x91]&2)
 visible=False
 for i,(a,b)in enumerate(zip(DX,DY)):
  found=at(x+a,y+b)
  if not found:continue
  a,b,t=found;own=owner(t,3)
  if i<9:
   if own==v or(mx>=0 and(mx,my)==(a,b)and vehicles[mid][14]==v):visible=True;break
   if pact(v,own):
    if any(vehicles[j][14]==v for j in ids(a,b)):visible=True;break
    if visible:break
  if sensor(a,b,t)and t[7]==v:visible=True;break
  if owner(t,1)==v or(mx>=0 and(mx,my)==(a,b)and radar(mid)):visible=True;break
  own=owner(t,2)
  if own==v:related=True
  elif own<0:
   if not sup:return [6]+[0]*8
   related=bool(negative&(1<<v))
  else:related=bool(pact(own,v))
  if related and any(radar(j)for j in ids(a,b)):visible=True
 if not visible and edition and exp:
  for a in range(-4,5):
   for b in range(-4,5):
    if a*a+b*b>=25:continue
    found=at(x+a-b,y+a+b)
    if found and owner(found[2],1)==v:
     for u in bases:
      if struct.unpack_from('<hh',u)==found[:2]and u[0x91]&2:visible=True
 t=tiles[(y*8+x)//2];lm=struct.unpack_from('<I',t,12)[0];return[0,struct.unpack_from('<I',t,12+v*4)[0],(t[0]>>3)&3,t[5]>>6,v,0,int(visible),lm&~0x200000 if visible else lm|0x200000,int(z< -6 or bool(draw&0x80000000))]

def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');a=a.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','rules_text','vehicle_stack','terrain_observation','terrain_visibility','terrain_visibility_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');combined=bytearray();records=[]
 for profile in range(11):
  w=fixture(profile);_,o,_=decode(w);f=out/f'fixture{profile}.SC';f.write_bytes(w);queries=[]
  for edition in range(2):
   for i in range(24):
    y=i//4;x=i%4*2+(y&1)
    for v in range(-1,8):
     for j in range(8):queries.append([x,y,v,[-8,-7,0,1][j%4],1,0,0x100 if j&1 else 0,1,0,edition,(j%3)*2 if j&2 else-1,0 if j&2 else-1,j%3 if j&2 else-1,int(bool(j&4)),0xaa if j&4 else 0,int(j!=3),0])
  data=b''.join(struct.pack('<17i',*q)for q in queries);expected=b''.join(struct.pack('<9I',*model(w,o,q,profile))for q in queries);command=runner+[str(exe),str(f)]+(['--synthetic-disabled-radar'if profile==10 else'--synthetic-gated-radar']if profile>=8 else[]);r=subprocess.run(command,input=data,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr)
  if r.stdout!=expected:
   for i,q in enumerate(queries):
    if r.stdout[i*36:(i+1)*36]!=expected[i*36:(i+1)*36]:raise AssertionError((profile,i,q,struct.unpack_from('<9I',r.stdout,i*36),model(w,o,q,profile)))
  combined+=r.stdout;records.append(dict(profile=profile,cases=len(queries)))
 bad=[];base=fixture(0);_,o,_=decode(base)
 def reject(label,offset,fmt,value,status):
  b=bytearray(base);struct.pack_into('<'+fmt,b,offset,value);bad.append((label,b,status))
 reject('missing-vehicle-bit',o['tiles']+8,'I',0,3)
 reject('cyclic-stack',o['vehicles']+50,'h',0,3)
 reject('missing-base-bit',o['tiles']+8*44+8,'I',0,3)
 reject('phantom-base-bit',o['tiles']+23*44+8,'I',1,3)
 reject('bad-chassis',o['prototypes']+320*52+36,'B',9,4)
 reject('bad-bonus-count',o['faction_definitions']+5*0x59c+0x50c,'i',9,4)
 for label,b,status in bad:
  f=out/(label+'.SC');f.write_bytes(b);q=[6,4,1,0,1,0,0,1,0,1,-1,-1,-1,0,0,1,0];run=subprocess.run(runner+[str(exe),str(f)],input=struct.pack('<17i',*q),capture_output=True,env=env);assert run.returncode==0,(label,run.returncode,run.stderr);assert run.stdout==struct.pack('<9I',status,*([0]*8)),(label,run.stdout)
 command=runner+[str(exe),str(out/'fixture0.SC')]
 for q in [[6,4,1,0,1,0,0,1,0,1,-2,-1,-1,0,0,1],[6,4,1,0,1,0,0,1,0,1,0,0,3,0,0,1],[6,4,1,0,1,0,0,1,0,2,-1,-1,-1,0,0,1]]:
  run=subprocess.run(command,input=struct.pack('<17i',*(q+[0])),capture_output=True,env=env);assert run.returncode==0 and run.stdout==struct.pack('<9I',2,*([0]*8)),run.stderr
 mismatch=[6,4,1,0,1,0,0,1,0,1,-1,-1,-1,0,0,1,1];held=subprocess.run(command,input=struct.pack('<17i',*mismatch),capture_output=True,env=env);assert held.returncode==0 and held.stdout==struct.pack('<9I',8,*([0]*8))
 assert subprocess.run(command,input=b'partial',capture_output=True,env=env).returncode==20
 digest=hashlib.sha256(combined).hexdigest();assert digest=='038fbd83bd5ca3ce6dcabb59dcde8a06c83c2ad56827a6378dc63f199dc23ac5',digest;(out/'output.bin').write_bytes(combined);record=dict(profiles=records,cases=sum(r['cases']for r in records),invalid=9,heldGeometry=1,aliasRejects=14,partialRejected=True,missingRuntimeTransactional=True,statusCounts={str(i):sum(struct.unpack_from('<I',combined,j)[0]==i for j in range(0,len(combined),36))for i in range(9)},bigEndian=a.m68k_linux,sha256=digest,native=False);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
