# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,subprocess,os,hashlib,json,argparse

def vectors():
 for alt in[0,1,2]:
  for z in[-8,-7,-5,5]:
   for viewer in[-1,1,7]:
    for using in[0,1,7]:
     for flags in[0,1,32,33,4096,4128]:
      for items in[0,0x8000,0x10,0x40,0x400000,0x800000,0x1000000,0x80000000,0x50,0x81c00050,0x81c08050,0x80008000]:
       for late in[0,1]:
        if viewer<0 and not late:continue
        for expansion in[0,1]:yield(viewer,7,using,11,9,z,alt,flags,items,0,late,expansion,1,1)
def thermal_variants():
 for raw in[0,0x1000,0x80001000]:
  for exp in[0,1]:yield(-1,7,0,11,9,-5,1,32,0x1000000,raw,1,exp,1,1)
def expected(q):
 viewer,current,using,x,y,z,alt,flags,items,lm,late,exp,normal,supplied=q;events=[];crop=[0]*5
 if late:
  if flags&1 and items&0x8000:events.append((1,0))
  if(flags&32 and items&0x8000 and z>-8)or flags&4096:events.append((2,0));crop=[viewer if viewer>=0 else using or current,-1,x,y,0]
  if flags&32 and z>-8:
   if items&0x80000000:events.append((3,0))
   for bit,event in[(0x10,4),(0x40,5),(0x400000,6),(0x800000,7),(0x1000000,8)]:
    if items&bit and(event>5 or exp or alt==2):events.append((event,int(event==8 and lm&0x80001000==0x1000)))
 return struct.pack('<7i',0,len(events),*crop)+b''.join(struct.pack('<2I',*e)for e in events)+bytes((8-len(events))*8)
def main():
 a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--cc',default='g++');a.add_argument('--m68k-linux',action='store_true');args=a.parse_args();out=Path(args.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-g'];runner=['qemu-m68k','-cpu','m68040']if args.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['probe','control']:
  f=out/name;subprocess.run([args.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'terrain_sea_late.cpp'),str(s/('terrain_sea_late_'+name+'.cpp')),'-o',str(f)],check=True);bins[name]=hashlib.sha256(f.read_bytes()).hexdigest()
 cases=list(vectors())+list(thermal_variants());packed=b''.join(struct.pack('<14I',*[v&0xffffffff for v in q])for q in cases);exp=b''.join(expected(q)for q in cases);r=subprocess.run(runner+[str(out/'probe')],input=packed,capture_output=True,env=env);(out/'stderr.txt').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==exp,(len(r.stdout),len(exp),next((i for i,(a,b)in enumerate(zip(r.stdout,exp))if a!=b),None));assert hashlib.sha256(r.stdout).hexdigest()=='e40c7d43b8ff30ac87baeb0ea5b690fa89d02e623c710980538c5710aa7040b2';c=subprocess.run(runner+[str(out/'control')],capture_output=True,env=env);(out/'control.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,(c.returncode,c.stdout,c.stderr);rec=dict(cases=len(cases),bytes=len(r.stdout),sha256=hashlib.sha256(r.stdout).hexdigest(),controls=c.stdout.decode().strip(),bigEndian=args.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
