# SPDX-License-Identifier: GPL-3.0-or-later
"""Ten-part assembly controls; all assets are generated synthetic fixtures."""
import argparse,os,subprocess,json
from pathlib import Path
from test_cvr_infantry import model
from test_cvr_lod import chain
NAMES=['vrcp00.cvr','vgmc.cvr','vgmcp.cvr','vb.cvr','vbp.cvr','vsp.cvr','vsptb.cvr','vsptf.cvr','vlights.cvr','vr00.cvr']
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined'];exe=out/'control'
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in ['cvr_model','cvr_raster','cvr_lighting','cvr_infantry','cvr_speeder_control']],'-lm','-o',str(exe)],check=True)
 cases=[];red=(255,0,0);blue=(0,0,255)
 for i in range(10):
  parts=[model(blue,frames=1)for _ in NAMES];parts[i]=model(red,frames=1)
  cases.append((f'tie-part-{i}',parts,int(i==0),int(i!=0),True))
 for i in range(10):
  parts=[model(red,frames=1,hidden=j!=i)for j in range(10)];cases.append((f'only-part-{i}',parts,1,0,True))
 parts=[model(red,frames=1)]+[model(blue,frames=1)for _ in range(9)];parts[-1]=model(blue,frames=1,offset=(100,0,0));cases.append(('last-part-distinct-palette',parts,1,1,True))
 for i in range(10):
  parts=[model(red,frames=1)for _ in NAMES];parts[i]=model(blue,frames=1,singular=True);cases.append((f'singular-{i}',parts,0,0,False))
 parts=[model(red,frames=1)for _ in NAMES];parts[8]=model(blue,frames=1,geom=chain((10,20)));cases.append(('unsupported-zoom-lod-change',parts,0,0,False))
 cases.append(('all-hidden',[model(red,frames=1,hidden=True)for _ in NAMES],0,0,False))
 cases.append(('missing-component',[model(red,frames=1)for _ in range(9)],0,0,False))
 report=[]
 for name,parts,r,b,valid in cases:
  d=out/name;d.mkdir()
  for p,data in zip(NAMES,parts):(d/p).write_bytes(data)
  cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(d),str(r),str(b),str(int(valid))]
  p=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert p.returncode==0,(name,p.returncode,p.stdout,p.stderr);report.append(dict(name=name,result=p.stdout.strip()))
 (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(f'SPEEDER {len(report)} composition controls PASS')
if __name__=='__main__':main()
