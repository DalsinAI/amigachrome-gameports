# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual owned-World water pass with explicit synthetic renderer light context."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode

def fixture():
 b=bytearray(valid_fixture());_,off,_=decode(b);start=off['tiles']
 for i in range(24):b[start+i*44]=((i*3)%3)<<5;b[start+i*44+1]=(i*13+7)%256
 return b

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];names=['original_map','original_game','original_faction','original_world','terrain_ocean_detail','terrain_water_texture','terrain_water_patch','terrain_water_patch_probe'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);world=out/'fixture.SC';world.write_bytes(fixture());atlas=out/'atlas.bin';atlas.write_bytes(bytes((x*17+y*31)%236+10 for y in range(768)for x in range(1024)));table=out/'light.bin';table.write_bytes(struct.pack('<80i',*[i*7%38 for i in range(80)]));cs=[((i%4)*2+((i//4)&1),i//4,-12+i%3,20-i%5,1+i%2,-1 if i%2 else 65,-256,256)for i in range(24)];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');command=runner+[str(exe),str(world),str(atlas),str(table)];r=subprocess.run(command,input=b''.join(struct.pack('<8i',*c)for c in cs),capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==24*16384;digest=hashlib.sha256(r.stdout).hexdigest();assert digest=='5fb3ed84ab4fd00084f4ccfc21caa81d499dfffc2846f9a8974aa83489cb68e8',digest;bad=subprocess.run(command,input=b'partial',capture_output=True,env=env);assert bad.returncode==20;(out/'output.bin').write_bytes(r.stdout);record=dict(patches=24,pixels=len(r.stdout),planRejects=9,planAliasRejects=11,renderRejects=2,renderAliasRejects=14,partialRejected=True,primaryClampExplicit=[-256,256],bigEndian=a.m68k_linux,sha256=digest);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
