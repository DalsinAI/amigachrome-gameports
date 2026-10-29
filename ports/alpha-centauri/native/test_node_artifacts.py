#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
HERE=Path(__file__).resolve().parent
NAMES=['original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','vehicle_stack.cpp','node_artifacts.cpp','node_artifacts_probe.cpp']
S=0x31415926

def fixture(version,n,case):
 b=bytearray(valid_fixture(version,n,1));info,off,spans=decode(b);bp=off['bases'];struct.pack_into('<hh',b,bp,2,2);struct.pack_into('<I',b,bp+0x30,[0,0xc00,0xffffffff,0x80000400][case]);b[bp+0x8d]|=1
 for i in range(len(spans['tiles'])//44):struct.pack_into('<I',b,off['tiles']+i*44+8,0)
 groups={};rng=random.Random(81817+n+case)
 for i in range(n):
  vp=off['vehicles']+i*52;x,y=(4,2) if case==2 or case==1 and i%2 else (2,2);struct.pack_into('<hh',b,vp,x,y);b[vp+14]=i%8;b[vp+17]=[0,1,17,255][i%4];struct.pack_into('<h',b,vp+10,i%512);b[off['prototypes']+(i%512)*52+42]=12 if i%3==0 else 0;groups.setdefault((x,y),[]).append(i)
 for (x,y),ids in groups.items():
  rng.shuffle(ids);tile=(y*int(info['width'])+x)//2;struct.pack_into('<I',b,off['tiles']+tile*44+8,2)
  for pos,i in enumerate(ids):struct.pack_into('<hh',b,off['vehicles']+i*52+48,ids[pos+1] if pos+1<len(ids) else -1,ids[pos-1] if pos else -1)
 return bytes(b)

def oracle(data,runtime,base):
 fail=struct.pack('<5I',1,S,S,S,S)+bytes([0x5a])*4096
 info,_,spans=decode(data);n=int(info['vehicles']);nb=int(info['bases']);bs=304 if int(info['version'])<11 else 308;w=int(info['width']);h=int(info['height'])
 if runtime or not 0<=base<nb:return fail
 b=spans['bases'][base*bs:(base+1)*bs]
 if not b[0x8d]&1:return fail
 actors=[spans['vehicles'][i*52:(i+1)*52] for i in range(n)];groups={};edges=[]
 for i,v in enumerate(actors):
  x,y=struct.unpack_from('<hh',v);nxt,prev=struct.unpack_from('<hh',v,48);edges.append((nxt,prev));groups.setdefault((x,y),set()).add(i)
  if not (0<=x<w and 0<=y<h and (x+y)%2==0 and -1<=nxt<n and -1<=prev<n):return fail
 for coords,ids in groups.items():
  roots=[i for i in ids if edges[i][1]==-1]
  if len(roots)!=1:return fail
  visited=set();i=roots[0];prev=-1
  while i!=-1:
   if i not in ids or i in visited or edges[i][1]!=prev:return fail
   visited.add(i);prev=i;i=edges[i][0]
  if visited!=ids:return fail
 occupied={(y*w+x)//2 for x,y in groups}
 for i in range(len(spans['tiles'])//44):
  if bool(struct.unpack_from('<I',spans['tiles'],i*44+8)[0]&2)!=(i in occupied):return fail
 xy=struct.unpack_from('<hh',b);ids=groups.get(xy,set());mask=bytearray(4096);artifacts=changed=0
 for i in ids:
  unit=struct.unpack_from('<h',actors[i],10)[0]
  if struct.unpack_from('<b',spans['prototypes'],unit*52+42)[0]==12:mask[i//8]|=1<<(i%8);artifacts+=1;changed+=actors[i][17]!=0
 flags=struct.unpack_from('<I',b,0x30)[0]&~0xc00
 return struct.pack('<5I',0,flags,len(ids),artifacts,changed)+mask

def cases():
 out=[]
 for version in range(7,13):
  for n in [0,1,2,3,8,17,64]:
   for case in range(4):out.append((f'v{version}-{n}-{case}',version,fixture(version,n,case)))
  b=bytearray(fixture(version,3,0));_,off,_=decode(b)
  for name,at,fmt,value in [('no-node',off['bases']+0x8d,'B',0),('cycle',off['vehicles']+48,'h',0),('split-stack',off['vehicles']+50,'h',-1),('missing-bit',off['tiles']+((2*8+2)//2)*44+8,'I',0),('stale-bit',off['tiles']+8,'I',2)]:
   q=bytearray(b);struct.pack_into('<'+fmt,q,at,value);out.append((f'v{version}-{name}',version,bytes(q)))
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 exe=out/'node-artifacts-probe';cmd=[a.cc,*flags,*[str(HERE/n) for n in NAMES],'-o',str(exe)];subprocess.run(cmd,check=True);digest=hashlib.sha256();records=[];accepted=rejected=artifacts=changes=0
 for name,version,data in cases():
  fp=out/(name+'.SC');fp.write_bytes(data);qs=[(0,0),(1,0),(0,-1),(0,1)];qp=out/(name+'.queries');qp.write_bytes(b''.join(struct.pack('<2i',*q) for q in qs));expected=[oracle(data,*q) for q in qs];want=b''.join(expected);r=subprocess.run([*runner,str(exe),'base' if version<11 else 'expansion',str(fp),str(qp)],capture_output=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);assert r.stdout==want,(name,len(r.stdout),len(want));digest.update(want)
  for x in expected:
   e,_,_,n,changed=struct.unpack_from('<5I',x);accepted+=e==0;rejected+=e!=0
   if not e:artifacts+=n;changes+=changed
  (out/(name+'.bin')).write_bytes(r.stdout);records.append(dict(name=name,inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest()))
 receipt=dict(worlds=len(records),queries=accepted+rejected,accepted=accepted,rejected=rejected,artifactTargets=artifacts,changedOrders=changes,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),records=records,command=cmd,scope='Read-only post-Node offline effect proposal; coherent stacks,foreign-faction artifacts,idempotent order0,cycles/bits/runtime/alias guards; World/opaque/RNG snapshots unchanged. No commit/completion/phase/UI.');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ('records','command')}))
if __name__=='__main__':main()
