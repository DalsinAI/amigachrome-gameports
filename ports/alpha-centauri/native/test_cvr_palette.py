# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent generated palette/tint oracle; no original game palette data."""
import argparse,os,struct,subprocess,json
from pathlib import Path

def table(palette,shades,tint):
 mapping=[]
 for i in range(256):
  color=[x*t//255 for x,t in zip(palette[i*3:i*3+3],tint)]
  distances=[sum((a-b)**2 for a,b in zip(palette[j*3:j*3+3],color))for j in range(256)]
  mapping.append(min(range(256),key=lambda j:distances[j]))
 return bytes(mapping[i]for i in shades)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined'];exe=out/'probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_palette.cpp'),str(src/'cvr_palette_probe.cpp'),'-o',str(exe)],check=True)
 palettes=[bytes(v for i in range(256)for v in ((i*17)%256,(i*37)%256,(i*67)%256)),bytes(v for i in range(256)for v in (i,i,i)),bytes([255,0,0])*256]
 tints=[(0,0,0),(255,255,255),(255,0,0),(0,255,0),(0,0,255),(128,173,227),(1,254,129),(247,211,19)]
 shades=bytes((i*13+i//256)%256 for i in range(6144));report=[]
 for j,pal in enumerate(palettes):
  for k,tint in enumerate(tints):
   data=b'ACPA'+struct.pack('<I',1)+pal+shades+bytes(tint);source=out/f'{j}-{k}.bin';dest=out/f'{j}-{k}.out';source.write_bytes(data)
   cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe),str(source),str(dest)];r=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert r.returncode==0,(j,k,r.stdout,r.stderr);assert dest.read_bytes()==table(pal,shades,tint),(j,k)
   report.append(dict(palette=j,tint=tint,entries=6144,invalidAPIControls=14))
 global_exe=out/'global-probe';subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_palette.cpp'),str(src/'cvr_global_palette_probe.cpp'),'-o',str(global_exe)],check=True)
 for j,pal in enumerate(palettes):
  source=out/f'global-{j}.bin';dest=out/f'global-{j}.out';source.write_bytes(pal)
  cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(global_exe),str(source),str(dest)];r=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);assert r.returncode==0,(j,r.stdout,r.stderr)
  actual=dest.read_bytes();assert actual[30:738]==bytes(min(v,235)for v in pal[:708]);assert len(actual)==768
  for n in range(8):
   assert list(actual[n*3:n*3+3])==[192 if n==7 else(128 if n&b else 0)for b in [1,2,4]]
   assert list(actual[(248+n)*3:(249+n)*3])==[128 if n==0 else(255 if n&b else 0)for b in [1,2,4]]
  for n,value in [(8,8),(9,9),(246,245),(247,246)]:assert list(actual[n*3:n*3+3])==[value,0,0]
 (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(f'CVR FACTION PALETTE {len(report)} tables {len(report)*6144} entries and14invalidcontrols;3default global palettes+6invalidcontrols PASS')
if __name__=='__main__':main()
