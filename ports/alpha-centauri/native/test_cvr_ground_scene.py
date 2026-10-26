# SPDX-License-Identifier: GPL-3.0-or-later
"""Exact slope caller state and complete synthetic no-shadow scene controls."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from test_cvr_source_scene import make_fixture

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 def build(name,names):
  exe=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in names],'-lm','-o',str(exe)],check=True);return exe
 def run(exe,args,name):
  r=subprocess.run(runner+[str(exe),*map(str,args)],capture_output=True,env=env,timeout=180);(out/(name+'.bin')).write_bytes(r.stdout);(out/(name+'.log')).write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr);return r
 exe=build('factory',['cvr_ground_scene','cvr_terrain_normal','cvr_ground_scene_probe']);r=run(exe,[],'factory');assert len(r.stdout)==19208*100;assert b'FACTORY13 invalid transactional controls PASS'in r.stderr;factorysha=hashlib.sha256(r.stdout).hexdigest();assert factorysha=='a94acef41edf4a22efb3e6b05b94aa399467ce0e728a94f24d487cef18c02edc',factorysha
 exe=build('render',['cvr_ground_scene','cvr_terrain_normal','cvr_scene','cvr_model','cvr_projection','cvr_fixed_transform','cvr_lighting','cvr_raster','cvr_ground_render_probe']);fixture=out/'synthetic';inf,rover=make_fixture(fixture,2,False);faction=fixture/'faction.bin';faction.write_bytes(bytes((c+7*s)&255 for s in range(24)for c in range(256)));contexts=[(4,4,4,5),(2,1,2,5),(1,2,5,2),(5,5,2,2)];goldens=json.loads((src/'GROUND-SCENE-GOLDENS-20261026.json').read_text());reports=[]
 for g in goldens:
  r=run(exe,[rover,inf,faction,*contexts[g['context']],g['zoom']],f"scene-{g['context']}-{g['zoom']}");digest=hashlib.sha256(r.stdout).hexdigest();assert len(r.stdout)==16*196608;assert digest==g['sceneSHA256'],(g,digest);assert b'alias rejections='in r.stderr and b'adjacent-factory-outputs=2 PASS'in r.stderr;reports.append(dict(context=g['context'],zoom=g['zoom'],cases=16,sha256=digest))
 (out/'receipt.json').write_text(json.dumps(dict(factoryCases=19208,factoryInvalidTransactional=13,factorySHA256=factorysha,syntheticCompleteScenes=128,sceneReports=reports,bigEndian=a.m68k_linux,ownerAssets=False,originalCodeExecuted=False),indent=2)+'\n');print('GROUND CALLER19208 exact states +128 complete synthetic scenes/alias controls PASS')
if __name__=='__main__':main()
