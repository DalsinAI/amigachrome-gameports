# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded non-flat facet and camera math; no map/asset/executable inputs."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_terrain_normal.cpp'),str(src/'cvr_terrain_normal_probe.cpp'),'-lm','-o',str(exe)],check=True);cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(exe)];r=subprocess.run(cmd,capture_output=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr.log').write_bytes(r.stderr);assert r.returncode==0,(r.returncode,r.stderr);digest=hashlib.sha256(r.stdout).hexdigest();assert len(r.stdout)==2401*64;assert digest=='84ee2c7e293d035ce1ca2197bcbda524051fc527d277ad5e1af2e60c84180dd0',digest;(out/'receipt.json').write_text(json.dumps(dict(cases=2401,exactOriginalNumericSHA256=digest,bigEndian=a.m68k_linux,publicInputsOnly=True,originalCodeExecuted=False,message=r.stderr.decode()),indent=2)+'\n');print(r.stderr.decode().strip())
if __name__=='__main__':main()
