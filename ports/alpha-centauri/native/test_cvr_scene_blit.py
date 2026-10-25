# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent destination-coordinate compositor oracle; no game media."""
import argparse,subprocess,os,json
from pathlib import Path

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'control';flags=['-m68040','-static']if a.m68k_linux else ['-g','-fsanitize=address,undefined'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp'))for n in ['pcx_image','cvr_scene_blit','cvr_scene_blit_control']],'-o',str(exe)],check=True)
 r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);(out/'stdout.log').write_text(r.stdout);(out/'stderr.log').write_text(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);rows=r.stdout.splitlines();assert len(rows)==76,len(rows)
 for row in rows[:75]:
  cx,cy,kind,h=row.split();cx=int(cx);cy=int(cy);kind=int(kind);clip=(0,0,400,300)if kind==400 else (53,49,311,251)if kind==311 else(0,0,0,0);pixels=bytearray([85])*(300*1216)
  for dy in range(clip[1],clip[3]):
   sy=dy-(cy-128)
   if not 0<=sy<256:continue
   for dx in range(clip[0],clip[2]):
    sx=dx-(cx-128)
    if not 0<=sx<256:continue
    c=(sx+sy*3)%255
    if c!=9:pixels[dy*1216+dx*3:dy*1216+dx*3+3]=bytes([c,(c*3)&255,(c*7)&255])
  expected=2166136261
  for value in pixels:expected=((expected^value)*16777619)&0xffffffff
  assert f'{expected:08x}'==h,(row,f'{expected:08x}')
 (out/'receipt.json').write_text(json.dumps(dict(cases=75,aliasRejections=10,invalidCases=4,legacyArithmeticCases=2,independentDestinationOracle=True,bigEndian=a.m68k_linux,originalWrapperExecuted=False))+'\n');print('SCENE BLIT75independent+10alias+4invalid+2legacy-arithmetic controls PASS')
if __name__=='__main__':main()
