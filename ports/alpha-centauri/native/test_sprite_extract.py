# SPDX-License-Identifier: GPL-3.0-or-later
"""Acquired RAM crop/trim controls with synthetic inputs only."""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess

def fixtures():
 for w,h in[(9,7),(19,13),(32,24)]:
  for key in[0,9,255]:
   for mode in range(8):
    for x,y,cw,ch in[(0,0,w,h),(2,1,w-3,h-2),(w-1,h-1,1,1)]:
     pitch=w+5;b=bytearray([key]*(pitch*h))
     for yy in range(ch):
      for xx in range(cw):
       paint=[False,0<xx<cw-1 and 0<yy<ch-1,xx==0 and yy==0,xx==cw-1 and yy==ch-1,(xx,yy)in[(0,ch-1),(cw-1,0)],(xx+yy)%2==0,yy%3==1,(xx*7+yy*13)%11<3][mode]
       if paint:
        c=(xx*17+yy*31+37)%256;b[(y+yy)*pitch+x+xx]=(c+1)%256 if c==key else c
     yield(w,h,pitch,key,x,y,cw,ch),bytes(b)
def reference(q,image):
 w,h,p,key,x,y,cw,ch=q;points=[(xx,yy)for yy in range(ch)for xx in range(cw)if image[(y+yy)*p+x+xx]!=key];pixels=bytearray([199]*(cw*ch))
 if not points:meta=(key,0,0,0,cw,ch,0,ch);status=9
 else:
  l=min(a for a,b in points);r=max(a for a,b in points)+1;t=min(b for a,b in points);bottom=max(b for a,b in points)+1;tw,th=r-l,bottom-t;trim=b''.join(image[(y+yy)*p+x+l:(y+yy)*p+x+r]for yy in range(t,bottom));pixels[:len(trim)]=trim;meta=(key,tw,tw,th,cw,ch,l,t);status=0
 return struct.pack('<9I',status,*meta)+pixels

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  f=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'sprite_extract.cpp'),str(s/('sprite_extract_'+name+'.cpp')),'-o',str(f)],check=True);bins[name]=hashlib.sha256(f.read_bytes()).hexdigest()
 cases=list(fixtures());packed=b''.join(struct.pack('<8i',*q)+image for q,image in cases);expected=b''.join(reference(*q)for q in cases);r=subprocess.run(runner+[str(out/'probe')],input=packed,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==expected,(len(r.stdout),len(expected),next((i for i,(x,y)in enumerate(zip(r.stdout,expected))if x!=y),None));h=hashlib.sha256(r.stdout).hexdigest();assert h=='a8b54ee77206fc0060fa9fa610500b7fafcb595868d81eceeefa48b688b16a3b',h;c=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);(out/'input.bin').write_bytes(packed);(out/'output.bin').write_bytes(r.stdout);rec=dict(cases=len(cases),bytes=len(r.stdout),sha256=h,controls=c.stdout.decode().strip(),bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
