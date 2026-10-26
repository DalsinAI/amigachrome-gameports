# SPDX-License-Identifier: GPL-3.0-or-later
"""Original-source shaded keyed forest quad; portable no-owner fixtures."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 points=[(-64,-64),(-1,0),(0,0),(63,63),(64,64),(5,32),(32,5),(32,58),(58,32),(16,16),(17,18),(18,17),(0,63),(63,0),(-4,40),(80,12)]
 return [[*a,*b,c]for a in points for b in points for c in[0,37,190,255]]+[[17,17,17+dx,17+dy,249]for dx in range(-8,9)for dy in range(-8,9)]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(s/'terrain_line.cpp'),str(s/'terrain_line_probe.cpp'),'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];qs=cases();packed=b''.join(struct.pack('<5i',*q)for q in qs);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(qs)*4096;h=hashlib.sha256(r.stdout).hexdigest();assert h=='be375131be6f8c61560dbc3abba98596fc44ec08e03209b640ad23a70240b480',h;assert subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env).returncode==20;record=dict(cases=len(qs),pixels=len(r.stdout),rejections=10,partialRejected=True,bigEndian=a.m68k_linux,native=False,sha256=h);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
