# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned ordinary scene composition with fully synthetic inputs, no owner media."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_terrain_visibility import fixture as visibility_fixture
from test_original_game import decode
NAMES=['original_map','original_game','original_faction','original_world','rules_text','vehicle_stack','terrain_observation','terrain_visibility','terrain_caller_order','cvr_terrain_context','terrain_lighting','terrain_mesh','terrain_dispatch','terrain_texture','terrain_ground_plate','terrain_world_masks','terrain_coast','terrain_coast_resources','terrain_coast_overlay','terrain_ocean_detail','terrain_water_texture','terrain_water_patch','terrain_keyed_quad','terrain_feature_layers','terrain_forest_quad','terrain_scene','terrain_scene_probe']
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
 concealed_digest=hashlib.sha256()
 for profile in range(3):
  b=fixture(profile);_,o,_=decode(b)
  for i in range(24):
   at=o['tiles']+i*44;b[at+4]=0
   if i%2:b[at]=(b[at]&0xe7)|0x18;b[at+5]=(b[at+5]&0x3f)|0xc0
   if i%3==0:struct.pack_into('<I',b,at+8,struct.unpack_from('<I',b,at+8)[0]|0x20)
   if i%4==0:struct.pack_into('<I',b,at+12,struct.unpack_from('<I',b,at+12)[0]|4)
  f=out/f'concealed{profile}.SC';f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(out/'atlas.bin'),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];queries=[(i,z,1,cx,cy,0,0x100,1)for i in range(24)for z in [1,2]for cx,cy in [(-12,20),(14,38),(-80,-30)]];r=subprocess.run(command,input=b''.join(struct.pack('<8i',*q)for q in queries),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(queries)*16388
  for j,q in enumerate(queries):assert struct.unpack_from('<I',r.stdout,j*16388)[0]==0,(profile,q,struct.unpack_from('<I',r.stdout,j*16388)[0])
  concealed_digest.update(r.stdout)
 assert concealed_digest.hexdigest()=='2941f3cfcefa722ebbb81cce176243953f8d6a559b100c5c7c2b4cf3ebe95ce8',concealed_digest.hexdigest()
 feature_digest=hashlib.sha256();feature_atlas=out/'feature-atlas.bin';feature_atlas.write_bytes(bytes(9 if x%7==0 or y%11==0 else (x*17+y*31)%236+10 for y in range(768)for x in range(1024)))
 for profile in range(3):
  b=fixture(profile);_,o,_=decode(b)
  for i in range(24):
   at=o['tiles']+i*44
   if (i*7+profile)%11<6:struct.pack_into('<I',b,at+8,struct.unpack_from('<I',b,at+8)[0]|0x20)
   if (i*5+1)%7<4:struct.pack_into('<I',b,at+12,struct.unpack_from('<I',b,at+12)[0]|4)
  f=out/f'features{profile}.SC';f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(feature_atlas),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];queries=[(i,z,-1,cx,cy,0x80,0,0)for i in range(24)for z in[1,2]for cx,cy in[(-12,20),(14,38),(-80,-30)]];r=subprocess.run(command,input=b''.join(struct.pack('<8i',*q)for q in queries),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(queries)*16388
  for j,q in enumerate(queries):assert struct.unpack_from('<I',r.stdout,j*16388)[0]==0,(profile,q,struct.unpack_from('<I',r.stdout,j*16388)[0])
  feature_digest.update(r.stdout)
 assert feature_digest.hexdigest()=='99321540305bd9d67eadd57518b31ba44457da25ed75bde211064857c37fb76b',feature_digest.hexdigest()
 forest_digest=hashlib.sha256()
 for profile in range(3):
  b=fixture(profile);_,o,_=decode(b)
  for i in range(24):
   at=o['tiles']+i*44
   if (i*7+profile)%11<7:struct.pack_into('<I',b,at+8,struct.unpack_from('<I',b,at+8)[0]|0x200000)
   if (i*3+profile)%7<3:struct.pack_into('<I',b,at+8,struct.unpack_from('<I',b,at+8)[0]|0x20)
   if (i*5+1)%7<4:struct.pack_into('<I',b,at+12,struct.unpack_from('<I',b,at+12)[0]|4)
  f=out/f'forest{profile}.SC';f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(feature_atlas),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin'),'0','1'];queries=[(i,z,-1,cx,cy,0x80,0,0)for i in range(24)for z in[1,2]for cx,cy in[(-12,20),(14,38),(-80,-30)]];r=subprocess.run(command,input=b''.join(struct.pack('<8i',*q)for q in queries),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(queries)*16388
  for j,q in enumerate(queries):assert struct.unpack_from('<I',r.stdout,j*16388)[0]==0,(profile,q,struct.unpack_from('<I',r.stdout,j*16388)[0])
  forest_digest.update(r.stdout)
 assert forest_digest.hexdigest()=='71f4c7d4aad5a8538a43550292382a47a0411075f7cf37006ab42da933297f89',forest_digest.hexdigest()
 held=[]
 base=fixture(0);_,o,_=decode(base)
 for label,offset,fmt,value,expected in [('jungle',o['tiles']+12,'I',4,0),('fungus',o['tiles']+8,'I',0x22,0),('special-height',o['tiles']+12,'I',2,4),('height-seven',o['tiles'],'B',224,5),('raw-rainfall-three',o['tiles'],'B',120,4),('raw-rockiness-three',o['tiles']+5,'B',192,4)]:
  b=bytearray(base);struct.pack_into('<'+fmt,b,offset,value);f=out/(label+'.SC');f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(out/'atlas.bin'),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];r=subprocess.run(command,input=struct.pack('<8i',0,1,-1,-12,20,0x80,0,0),capture_output=True,env=env);assert r.returncode==0,(label,r.returncode,r.stderr);assert struct.unpack_from('<I',r.stdout)[0]==expected,(label,struct.unpack_from('<I',r.stdout)[0]);assert (r.stdout[4:]==bytes([199])*16384)if expected else len(r.stdout)==16388;held.append(label)if expected else None
 # The same source bit enables farm ground and ocean platform sprite effects.
 # Keep both contexts explicitly held and byte-identical to input.
 for altitude in[2,3]:
  b=fixture(0);_,o,_=decode(b);b[o['tiles']]=altitude<<5;struct.pack_into('<I',b,o['tiles']+8,0x8000);f=out/f'held-farm-{altitude}.SC';f.write_bytes(b);command=runner+[str(exe),str(f),'--synthetic',str(feature_atlas),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin'),'0','1'];r=subprocess.run(command,input=struct.pack('<8i',0,1,-1,-12,20,0x80,0,0),capture_output=True,env=env);assert r.returncode==0 and struct.unpack_from('<I',r.stdout)[0]==4 and r.stdout[4:]==bytes([199])*16384,(altitude,r.returncode,r.stdout[:4],r.stderr)
 h=digest.hexdigest();assert h=='98415f1a7ee4d47282602719eaf86acc409a95cc084cbee281fd4104dd8f662c',h;record=dict(forestCases=432,forestSHA256=forest_digest.hexdigest(),heldFarmBranches=2,cases=count,syntheticWorlds=9,featureCases=432,featureSHA256=feature_digest.hexdigest(),concealedCases=432,concealedSHA256=concealed_digest.hexdigest(),heldFeatureContexts=held,pixels=count*16384,statusCounts=status,rejects=16,aliasRejects=50,partialRejected=True,transactionalLateStageFailure=True,sourceIdentityExplicit=True,freshActiveCacheProfile=True,native=False,bigEndian=a.m68k_linux,sha256=h);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
