# SPDX-License-Identifier: GPL-3.0-or-later
"""Forest four-neighbour variants and jungle/forest/fungus source ordering."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path

def cases():
 return [[x,y,0,1,3,2,0,z,mask*17,255-mask*17,3,bool(flags&1),bool(flags&4),hidden,visible,bool(flags&2),mask,-4,2,4,-1]for mask in range(16)for z in[1,2]for x,y in[(-12,8),(14,20)]for flags in range(8)for hidden in[0,1]for visible in[0,1]]
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;exe=out/'probe';flags=['-m68040','-static']if a.m68k_linux else['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in['terrain_mesh','terrain_keyed_quad','terrain_forest_quad','terrain_feature_layers','terrain_forest_layers_probe']],'-o',str(exe)],check=True);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');qs=cases();packed=b''.join(struct.pack('<21i',*q)for q in qs);r=subprocess.run(runner+[str(exe)],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(qs)*4096;h=hashlib.sha256(r.stdout).hexdigest();assert h=='03cb532e780883342116c28b64098a65b20eb56ae83c5933ba23db4b7e7ec6e2',h;assert subprocess.run(runner+[str(exe)],input=b'partial',capture_output=True,env=env).returncode==20;record=dict(cases=len(qs),forestMasks=16,allLayerBooleanOrders=8,pixels=len(r.stdout),rejections=10,aliasRejects=10,bigEndian=a.m68k_linux,native=False,sha256=h);(out/'receipt.json').write_text(json.dumps(record,indent=2)+'\n');print(record)
if __name__=='__main__':main()
