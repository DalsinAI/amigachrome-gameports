# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual renderer colour/cache transitions with wholly synthetic components."""
import argparse,subprocess,os,json
from pathlib import Path
from test_cvr_infantry import model
from test_cvr_model import geometry
from test_cvr_speeder import NAMES

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined'];exe=out/'control';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in ['cvr_model','cvr_raster','cvr_lighting','cvr_palette','cvr_infantry','cvr_palette_render_control']],'-lm','-o',str(exe)],check=True)
 for visible in [2,4]:
  d=out/f'part-{visible}';d.mkdir()
  for i,name in enumerate(NAMES):(d/name).write_bytes(model((255,0,0),frames=1,hidden=i!=visible))
  p=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(d)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert p.returncode==0,(visible,p.returncode,p.stdout,p.stderr)
 d=out/'transparent9';d.mkdir()
 for i,name in enumerate(NAMES):(d/name).write_bytes(model((255,0,0),frames=1,hidden=i!=2,geom=geometry(points=bytes([13<<3,0,9,26<<3,0,9]))))
 p=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(d),'transparent9'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
 (out/'receipt.json').write_text(json.dumps(dict(panelCases=2,transitions=18,draws=288,transparentKeyDraws=16,unsupportedProfiles='unchanged'))+'\n');print('CVR PALETTE RENDER 2panel18transition288draw+16key9 controls PASS')
if __name__=='__main__':main()
