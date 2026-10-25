# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned original CVR group metadata; no original assets required."""
from pathlib import Path
import argparse,struct,subprocess,os,json
from test_cvr_model import fixture,geometry

def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');a=p.parse_args();out=Path(a.out);out.mkdir(exist_ok=False,parents=True);src=Path(__file__).resolve().parent
 exe=out/'groups';subprocess.run([a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(src/'cvr_model.cpp'),str(src/'cvr_groups_probe.cpp'),'-o',str(exe)],check=True)
 def group(seed,offset,d):return struct.pack('<12hII',*seed,0,0,0,0,0,0,*offset,2,0)+bytes([d<<3,0,5,26<<3,0,5])
 def grouped(body,declared=2,position=(-1,-1,-1),dimensions=(3,3,3)):
  n=42+len(body)+5;return fixture(10,geom=struct.pack('<IIfII9hI',10,n,2,n,n-1,*dimensions,*position,0,0,0,declared)+body+b'\0'*5)
 cases=[('single',fixture(),[(0,0,0,0,1,2)]),('duplicate-position-distinct-seeds',grouped(group((0,0,0),(0,0,0),13)+group((1,0,0),(0,0,0),5)),[(0,0,0,0,1,2),(1,0,0,1,1,2)]),('seed32768',grouped(group((32767,0,0),(1,0,0),4),1,(32767,0,0),(1,1,1)),[(32768,0,0,0,1,2)]),('seed-minus32769',grouped(group((-32768,0,0),(-1,0,0),21),1,(-32768,0,0),(1,1,1)),[(-32769,0,0,0,1,2)])]
 for name,data,want in cases:
  f=out/(name+'.cvr');f.write_bytes(data);r=subprocess.run([str(exe),str(f)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);words=struct.unpack('<'+'i'*(len(r.stdout)//4),r.stdout);assert words[0]==1;count=words[2];got=[words[12+i*18:18+i*18]for i in range(count)];assert got==want,(name,got,want)
 malformed=[cases[1][1][:-1],fixture(10,geom=geometry(10,jump_flags=1)),grouped(group((0,0,0),(0,0,0),31),1)]
 for i,data in enumerate(malformed):
  f=out/f'bad{i}.cvr';f.write_bytes(data);r=subprocess.run([str(exe),str(f)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==20,(i,r.returncode,r.stderr)
 (out/'receipt.json').write_text(json.dumps(dict(valid=4,malformed=3,ownedAndTransactional=True))+'\n');print('CVR GROUPS 4owned/widened-seed +3malformed controls PASS')
if __name__=='__main__':main()
