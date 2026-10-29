# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
import test_ready_search as ready
import test_local_unit_wake as wake
HERE=Path(__file__).resolve().parent
SOURCES=wake.SOURCES[:-1]+['local_unit_wait.cpp','local_unit_wait_probe.cpp']
def fixture(version,case):
 text,data,oldfacts=ready.fixture(version,case);b=bytearray(data);info,o,_=ready.skip.resumed.workers.e.population.decode(b);b[o['vehicles']+14]=1
 return text,bytes(b),(0,oldfacts[1],oldfacts[2],0)
def model(text,data,facts):
 b=bytearray(data);_,o,_=ready.skip.resumed.workers.e.population.decode(b);p=o['vehicles']+facts[3]*52+4;struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]|2)
 return ready.model(text,bytes(b),facts)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in range(24):
   text,data,facts=fixture(version,case);selected,distance,passes,bitmap,expected=model(text,data,facts);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';fp=out/f'{i}.facts';rp.write_bytes(text);sp.write_bytes(data);fp.write_bytes(struct.pack('<4i',*facts));r=subprocess.run([*runner,str(exe),str(rp),str(sp),str(fp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);assert r.returncode==0 and struct.unpack_from('<iiI',r.stdout,4)==(selected,distance,passes) and r.stdout[16:272]==bitmap and r.stdout[272:]==expected,(version,case,r.returncode,r.stderr,r.stdout[:16],selected,distance,passes,next((j for j,(x,y) in enumerate(zip(r.stdout[272:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,selected=selected,passes=passes,cleared=sum(x.bit_count() for x in bitmap),guards=struct.unpack_from('<I',r.stdout)[0]))
 receipt=dict(worlds=len(records),selected=sum(r['selected']>=0 for r in records),none=sum(r['selected']<0 for r in records),cleared=sum(r['cleared'] for r in records),guards=sum(r['guards'] for r in records),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
