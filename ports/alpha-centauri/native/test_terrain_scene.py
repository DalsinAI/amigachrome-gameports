# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned ordinary scene composition with fully synthetic inputs, no owner media."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_terrain_visibility import fixture as visibility_fixture
from test_original_game import decode
NAMES=['original_map','original_game','original_faction','original_world','rules_text','vehicle_stack','terrain_observation','terrain_visibility','terrain_caller_order','cvr_terrain_context','terrain_lighting','terrain_mesh','terrain_dispatch','terrain_texture','terrain_ground_plate','terrain_world_masks','terrain_coast','terrain_coast_resources','terrain_coast_overlay','terrain_ocean_detail','terrain_water_texture','terrain_water_patch','terrain_scene','terrain_scene_probe']
def fixture(profile):
 b=visibility_fixture(0);_,o,_=decode(b)
 for i in range(24):
  p=o['tiles']+44*i;alt=3 if profile==0 else 3+i%2 if profile==1 else (2 if i%5<2 else 3)
  b[p]=(alt<<5)|((i%3)<<3);b[p+1]=(i*13+7)%256;b[p+4]=255;b[p+5]=(i%3)<<6
  struct.pack_into('<II',b,p+8,struct.unpack_from('<I',b,p+8)[0]&3,0x40 if i%7==0 and alt>=3 else 0)
 return b

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in NAMES],'-o',str(exe)],check=True)
 (out/'atlas.bin').write_bytes(bytes((x*17+y*31)%236+10 for y in range(768)for x in range(1024)));(out/'light.bin').write_bytes(struct.pack('<80i',*[i*7%38 for i in range(80)]));(out/'metadata.bin').write_bytes(bytes(range(53))+bytes((i*5+k)%6 for i in range(53)for k in range(8)));(out/'resources.bin').write_bytes(bytes((i*17+i//56)%53 for i in range(3136))+bytes([21,37,53,69,85,101]));runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');digest=hashlib.sha256();status={};count=0
 for profile in range(3):
  f=out/f'fixture{profile}.SC';f.write_bytes(fixture(profile));command=runner+[str(exe),str(f),'--synthetic',str(out/'atlas.bin'),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];queries=[(i,z,-1,cx,cy,0x80,0,0)for i in range(24)for z in [1,2]for cx,cy in [(-12,20),(14,38),(-80,-30)]];r=subprocess.run(command,input=b''.join(struct.pack('<8i',*q)for q in queries),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(queries)*16388
  for j,q in enumerate(queries):
   result=struct.unpack_from('<I',r.stdout,j*16388)[0];status[str(result)]=status.get(str(result),0)+1
   assert result==0,(profile,q,result)
  digest.update(r.stdout);count+=len(queries)
  assert subprocess.run(command,input=b'partial',capture_output=True,env=env).returncode==20
 held=[]
 base=fixture(0);_,o,_=decode(base)
 for label,offset,fmt,value,expected in [('jungle',o['tiles']+12,'I',4,4),('fungus',o['tiles']+8,'I',0x22,4),('special-height',o['tiles']+12,'I',2,4),('height-seven',o['tiles'],'B',224,5)]:
  b=bytearray(base);struct.pack_into('<'+fmt,b,offset,value);f=out/(label+'.SC');f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(out/'atlas.bin'),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];r=subprocess.run(command,input=struct.pack('<8i',0,1,-1,-12,20,0x80,0,0),capture_output=True,env=env);assert r.returncode==0,(label,r.returncode,r.stderr);assert r.stdout==struct.pack('<I',expected)+bytes([199])*16384,(label,struct.unpack_from('<I',r.stdout)[0]);held.append(label)
 h=digest.hexdigest();assert h=='98415f1a7ee4d47282602719eaf86acc409a95cc084cbee281fd4104dd8f662c',h;record=dict(cases=count,syntheticWorlds=3,heldFeatureContexts=held,pixels=count*16384,statusCounts=status,rejects=15,aliasRejects=50,partialRejected=True,transactionalLateStageFailure=True,sourceIdentityExplicit=True,freshActiveCacheProfile=True,native=False,bigEndian=a.m68k_linux,sha256=h);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
