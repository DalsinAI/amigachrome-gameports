#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,itertools,json,os,random,shlex,struct,subprocess

def model(h):
 v,runtime,choice,upkeep,warnings,mirror,mask,gov,disabled=h
 if v not in (0,1) or runtime!=1 or disabled not in (0,1):return (1,0x12345678,0x12345678,0x12345678,0x12345678,1,1)
 staple=save=0
 if disabled:warnings&=~mask;mirror=warnings;save=1
 elif choice==0:upkeep=1
 elif choice==2:gov|=0x80000040
 elif choice==3:staple=1
 return (0,upkeep&0xffffffff,warnings,mirror,gov,staple,save)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;rng=random.Random(812712);rows=[];answers=[]
 for v,choice,upkeep,warnings,mask,gov,disabled in itertools.product(range(2),[-2147483648,-1,0,1,2,3,4,2147483647],[-2147483648,-1,0,1,2147483647],[0,1,8,0x2000,0x2001,0x80000000,0xffffffff],[0,1,8,0x2000,0x2001,0x80000000,0xffffffff],[0,1,64,0x80000000,0xffffffff],range(2)):
  h=[v,1,choice,upkeep,warnings,rng.randrange(1<<32),mask,gov,disabled];rows.append(struct.pack('<9I',*[x&0xffffffff for x in h]));answers.append(model(h))
 good=[0,1,2,0,0xffffffff,0,1,0,0]
 for field,values in [(0,[2,0xffffffff]),(1,[0,2,0xffffffff]),(8,[2,0xffffffff])]:
  for value in values:
   h=good.copy();h[field]=value;rows.append(struct.pack('<9I',*[x&0xffffffff for x in h]));answers.append(model(h))
 raw=b''.join(rows);want=b''.join(struct.pack('<7I',*x) for x in answers);(out/'inputs.bin').write_bytes(raw);cxx=shlex.split(a.cc);cross=a.m68k_linux or 'm68k' in cxx[0];cmd=cxx+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if cross else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/'popup_response.cpp'),str(src/'popup_response_probe.cpp'),'-o',str(out/'probe')];subprocess.run(cmd,check=True);run=(['qemu-m68k','-cpu','m68040'] if cross else [])+[str(out/'probe'),str(out/'inputs.bin')];r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=180);assert r.returncode==0,(r.returncode,r.stderr);assert r.stdout==want;(out/'outputs.bin').write_bytes(r.stdout);receipt=dict(cases=len(rows),rejected=sum(x[0]!=0 for x in answers),stapleRequired=sum(not x[0] and x[5] for x in answers),preferencesSaveRequired=sum(not x[0] and x[6] for x in answers),inputSHA256=hashlib.sha256(raw).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
