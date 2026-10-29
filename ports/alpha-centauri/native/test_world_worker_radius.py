#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode
from test_original_world import valid_fixture
from test_original_map import fixture as map_fixture
from worker_radius_reference import corpus,radius
HERE=Path(__file__).resolve().parent
def fixture(c,version):
 b=bytearray(valid_fixture(version,len(c['actors']),len(c['bases'])));_,off,spans=decode(b);newmap=map_fixture(c['width'],c['height'])[15:];b[off['metadata']:off['abstract']+len(spans['abstract'])]=newmap;_,off,_=decode(b);legacy=version<11;fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308;pf=off['legacy'] if legacy else off['preferences']
 struct.pack_into('<I',b,pf+0x30,0x10|(0x80 if c['omniscient'] else 0));struct.pack_into('<I',b,off['metadata']+28,c['flat']);faction=c['bases'][0][2];bit=1<<faction;struct.pack_into('<I',b,off['factions']+fs*faction,0x200 if c['reveal'] else 0)
 for i in range(8):struct.pack_into('<I',b,off['factions']+fs*faction+0x18+i*4,c.get('diplomacy',[0,0,c.get('diplo',0),0,0,0,0,0])[i])
 for i,(x,y,f,worked) in enumerate(c['bases']):
  p=off['bases']+i*bs;b[p:p+bs]=bytes(bs);struct.pack_into('<hhb',b,p,x,y,f);struct.pack_into('<I',b,p+0x78,worked)
 for i,(x,y,f,order,visible) in enumerate(c['actors']):
  p=off['vehicles']+52*i;b[p:p+52]=bytes(52);struct.pack_into('<hh',b,p,x,y);b[p+14]=f;b[p+17]=order;b[p+39]=bit if visible else 0;struct.pack_into('<hhh',b,p+46,-1,i+1 if i+1<len(c['actors']) else -1,i-1)
 for i,(alt,val,vis,owner,items) in enumerate(c['tiles']):
  p=off['tiles']+44*i;b[p:p+44]=bytes(44);b[p]=alt;b[p+2]=val;b[p+4]=vis;b[p+7]=owner&255;struct.pack_into('<I',b,p+8,items)
 return bytes(b)
def wire_expected(data):
 info,off,spans=decode(data);legacy=int(info['version'])<11;width,height=int(info['width']),int(info['height']);fs=0x20bc if legacy else 0x20cc;bs=304 if legacy else 308
 bases=[(*struct.unpack_from('<hhb',spans['bases'],i*bs),struct.unpack_from('<I',spans['bases'],i*bs+0x78)[0]) for i in range(int(info['bases']))]
 if not bases:return [1]+[0x5a5a5a5a]*26
 faction=bases[0][2];bit=1<<faction;tiles=[(spans['tiles'][p],spans['tiles'][p+2],spans['tiles'][p+4],struct.unpack_from('<b',spans['tiles'],p+7)[0],struct.unpack_from('<I',spans['tiles'],p+8)[0]) for p in range(0,len(spans['tiles']),44)]
 actors=[];links=[];heads={}
 for i in range(int(info['vehicles'])):
  p=i*52;x,y=struct.unpack_from('<hh',spans['vehicles'],p);owner=spans['vehicles'][p+14];order=spans['vehicles'][p+17];visible=bool(spans['vehicles'][p+39]&bit);nxt,prev=struct.unpack_from('<hh',spans['vehicles'],p+48);actors.append((x,y,owner,order,visible));links.append((nxt,prev))
  if prev==-1:
   if (x,y) in heads:return [3]+[0x5a5a5a5a]*26
   heads[x,y]=i
 for i,v in enumerate(actors):
  x,y,owner,_,_=v;nxt,prev=links[i]
  if not(0<=x<width and 0<=y<height and not((x+y)&1) and owner<8 and -1<=nxt<len(actors) and -1<=prev<len(actors)):return [3]+[0x5a5a5a5a]*26
  if nxt>=0 and (links[nxt][1]!=i or actors[nxt][:2]!=(x,y)):return [3]+[0x5a5a5a5a]*26
  at=i;seen=set()
  while links[at][1]>=0:
   if at in seen:return [3]+[0x5a5a5a5a]*26
   seen.add(at);prior=links[at][1]
   if prior>=len(actors) or links[prior][0]!=at or actors[prior][:2]!=(x,y):return [3]+[0x5a5a5a5a]*26
   at=prior
  if heads.get((x,y))!=at:return [3]+[0x5a5a5a5a]*26
 for i,tile in enumerate(tiles):
  y,k=divmod(i,width//2);x=k*2+(y&1)
  if bool(tile[4]&2)!=((x,y) in heads):return [3]+[0x5a5a5a5a]*26
 prefs=spans['legacy'] if legacy else spans['preferences'];f=spans['factions'][fs*faction:fs*(faction+1)];c=dict(width=width,height=height,flat=bool(struct.unpack_from('<I',spans['metadata'],28)[0]),bases=bases,tiles=tiles,actors=actors,reveal=bool(struct.unpack_from('<I',f)[0]&0x200),omniscient=bool(struct.unpack_from('<I',prefs,0x30)[0]&0x80),diplomacy=struct.unpack_from('<8I',f,0x18))
 try:return [0,*radius(c)]
 except ValueError:return [1]+[0x5a5a5a5a]*26
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-output',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','vehicle_stack.cpp','world_worker_radius.cpp','world_worker_radius_probe.cpp'];exe=a.out/'world-worker-radius-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in sources],'-o',str(exe)];subprocess.run(cmd,check=True)
 digest=hashlib.sha256();original=bytearray();runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];records=[]
 for i,c in enumerate(corpus()):
  data=fixture(c,7+i%6);p=a.out/f'{i}.SC';p.write_bytes(data);r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(i,r.returncode,r.stderr)
  result=radius(c);assert wire_expected(data)==[0,*result];want=struct.pack('<27I',0,*result);assert r.stdout==want,(i,struct.unpack('<27I',r.stdout),[0,*result]);assert p.read_bytes()==data;digest.update(want);original+=want[4:];records.append({'case':i,'version':7+i%6,'passed':True})
 if a.original_output:assert bytes(original)==a.original_output.read_bytes()
 rejects=[]
 first=next(corpus());last=list(corpus())[-1]
 for name,kind in [('phantom','phantom'),('missing','missing'),('cycle','cycle'),('two-roots','roots'),('owner','owner')]:
  data=bytearray(fixture(first if kind in ['phantom','owner'] else last,12));_,off,_=decode(data)
  if kind=='phantom':struct.pack_into('<I',data,off['tiles']+8,2)
  elif kind=='owner':
   x,y=first['bases'][0][:2];p=off['tiles']+44*((y*first['width']+x)//2);data[p+7]=9;struct.pack_into('<I',data,off['factions']+0x20cc,0x200)
  elif kind=='cycle':struct.pack_into('<h',data,off['vehicles']+48,0)
  elif kind=='roots':struct.pack_into('<h',data,off['vehicles']+52+50,-1)
  else:
   x,y=last['actors'][0][:2];p=off['tiles']+44*((y*last['width']+x)//2);struct.pack_into('<I',data,p+8,struct.unpack_from('<I',data,p+8)[0]&~2)
  rejects.append((name,bytes(data)))
 for f in range(8):
  for seed in [first,last]:
   c=json.loads(json.dumps(seed));c['bases'][0][2]=f;c['reveal']=True;c['omniscient']=bool(f&1);rejects.append((f'faction-{f}-actors{len(c["actors"])}',fixture(c,7+f%6)))
 for path in a.original:rejects.append(('original-'+path.name,path.read_bytes()))
 extra=hashlib.sha256();accepted_extra=rejected_extra=0
 for i,(name,data) in enumerate(rejects):
  p=a.out/(name+'.SC');p.write_bytes(data);r=subprocess.run([*runner,str(exe),str(p)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);want=struct.pack('<27I',*wire_expected(data));assert r.stdout==want,(name,r.stdout,want);assert p.read_bytes()==data;extra.update(want);accepted_extra+=wire_expected(data)[0]==0;rejected_extra+=wire_expected(data)[0]!=0
 (a.out/'outputs.bin').write_bytes(original);receipt={'worlds':len(records),'originalFlagsBytes':len(original),'originalFlagsSHA256':hashlib.sha256(original).hexdigest(),'outputSHA256':digest.hexdigest(),'command':cmd,'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'perWorldInvalidContexts':3,'wholeWorldAndWireByteComparedUnchanged':True,'extraContexts':len(rejects),'extraAccepted':accepted_extra,'extraRejected':rejected_extra,'extraSHA256':extra.hexdigest(),'actualAmigaOS':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
