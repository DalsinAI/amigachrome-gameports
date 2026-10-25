#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def fixture():return '#RESOURCEINFO\n'+''.join(f'row{i}, {i}, {i+1}, {i+2}, 0\n' for i in range(9))+'#END\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 src=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'resource-probe';command=[a.cc,*flags,str(src/'rules_text.cpp'),str(src/'resource_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
 data=fixture();cases=[('valid',data,True),('crlf',data.replace('\n','\r\n'),True),('cr',data.replace('\n','\r'),True),('lower',data.replace('RESOURCEINFO','resourceinfo'),True),('comments',data.replace('row0',';comment\nrow0'),True),('negative',data.replace('row0, 0, 1, 2, 0','row0, -2147483648, 2147483647, -2, -1'),True)]
 cases += [('missing','',False),('wrong','#WRONG\n',False),('short',data.replace('row8, 8, 9, 10, 0\n',''),False),('extra',data.replace('#END','row9, 1,2,3,4\n#END'),False),('nul',data.replace('row0','ro\0w0'),False),('long',data.replace('row0','a'*513),False),('label',data.replace('row0,',','),False)]
 for column in range(4):
  for token in ['', 'oops','2147483648','-2147483649']:
   row=['0','1','2','0'];row[column]=token;cases.append((f'bad-{column}-{token}',data.replace('row0, 0, 1, 2, 0','row0, '+','.join(row)),False))
 for tail in [',1',',,1',',oops']:
  cases.append(('annotation'+tail,data.replace('row0, 0, 1, 2, 0','row0, 0, 1, 2, 0'+tail),True))
 cases += [('wildcards',data.replace('row5, 5, 6, 7, 0','row5, 5, *, *, 0'),True),('wrong-wildcard',data.replace('row0, 0, 1, 2, 0','row0, *, 1, 2, 0'),False)]
 for i,(name,text,valid) in enumerate(cases):
  file=out/(str(i)+'.txt');file.write_text(text)
  r=subprocess.run([*runner,str(exe),str(file)],capture_output=True,text=True,timeout=10,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  assert r.returncode==(0 if valid else 20),(name,r.returncode,r.stdout,r.stderr)
  if valid:
   rows=[[int(v) for v in line.split()] for line in r.stdout.splitlines()];expected=[[i,i,i+1,i+2,0] for i in range(9)]
   if name=='negative':expected[0]=[0,-2147483648,2147483647,-2,-1]
   if name=='wildcards':expected[5]=[5,5,0,0,0]
   assert rows==expected,(name,rows,expected)
 (out/'receipt.json').write_text(json.dumps(dict(cases=len(cases),command=command,runner=runner,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(f'{len(cases)} resource cases passed')
if __name__=='__main__':main()
