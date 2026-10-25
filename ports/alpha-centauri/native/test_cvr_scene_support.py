# SPDX-License-Identifier: GPL-3.0-or-later
"""Projection/fixed/group/scene-alias support controls with no original assets."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from test_cvr_source_scene import make_fixture

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');golden=json.loads((src/'SOURCE-SYNTHETIC-GOLDENS-20261025.json').read_text());reports=[]
 def build(name,sources):
  exe=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in sources],'-lm','-o',str(exe)],check=True);return exe
 def run(exe,args=()):
  r=subprocess.run(runner+[str(exe),*map(str,args)],env=env,capture_output=True,timeout=120);(out/(exe.name+'-'+str(len(reports))+'.stdout')).write_bytes(r.stdout);(out/(exe.name+'-'+str(len(reports))+'.stderr')).write_bytes(r.stderr);assert r.returncode==0,(exe.name,r.returncode,r.stderr);reports.append(dict(name=exe.name,bytes=len(r.stdout),sha256=hashlib.sha256(r.stdout).hexdigest(),message=r.stderr.decode().strip()));return r
 p=build('projection',['cvr_projection','cvr_projection_control']);r=run(p);assert b'26 valid/invalid' in r.stdout,r.stdout
 p=build('groups-fixed',['cvr_fixed_transform','cvr_groups_fixed_control']);r=run(p);assert b'4widened+2same-position-distinct-seed+2invalid' in r.stdout,r.stdout
 p=build('fixed-object',['cvr_fixed_transform','cvr_lighting','cvr_raster','cvr_fixed_object_control']);r=run(p);assert b'8normal/no-shadow cases+16invalid' in r.stdout,r.stdout;r=run(p,['bytes']);assert hashlib.sha256(r.stdout).hexdigest()==golden['fixedObjectSHA256'],reports[-1]
 p=build('scene-alias',['cvr_model','cvr_projection','cvr_fixed_transform','cvr_lighting','cvr_raster','cvr_scene','cvr_scene_probe']);d=out/'fixture';inf,rover=make_fixture(d,2,False)
 palette=(d/'palette.bin').read_bytes();tint=(d/'tint.bin').read_bytes();remap=[]
 for c in range(256):
  rgb=[palette[c*3+i]*tint[i]//255 for i in range(3)];remap.append(min(range(256),key=lambda k:sum((palette[k*3+i]-rgb[i])**2 for i in range(3))))
 (d/'faction.bin').write_bytes(bytes(remap[(c+7*s)&255]for s in range(24)for c in range(256)));r=run(p,[rover,inf,d/'faction.bin']);assert len(r.stdout)==16*196608;assert hashlib.sha256(r.stdout).hexdigest()==golden['sceneAliasSHA256'],reports[-1];assert b'alias rejections='in r.stderr and b'adjacent-factory-outputs=2 PASS'in r.stderr,r.stderr
 (out/'receipt.json').write_text(json.dumps(dict(controls=reports,bigEndian=a.m68k_linux,ownerAssetsUsed=False,originalBytesUsed=False),indent=2)+'\n');print('SCENE SUPPORT projection26/fixed24/group8 +16 exact scene/alias controls PASS')
if __name__=='__main__':main()
