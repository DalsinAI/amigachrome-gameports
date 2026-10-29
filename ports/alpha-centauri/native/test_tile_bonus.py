#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from tile_bonus_reference import bonus_model,bonus_cases
HERE=Path(__file__).resolve().parent
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=a.out/'tile-bonus-probe';cmd=[a.cc,*flags,str(HERE/'tile_yields.cpp'),str(HERE/'tile_bonus_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True)
 rows=[list(r) for r in bonus_cases()];first=rows[0]
 for index,value in [(0,0),(0,1),(0,3),(0,4098),(1,0),(1,4097),(3,-1),(3,64),(4,-1),(4,64),(3,1),(6,256),(8,3),(8,1),(8,2)]:r=first[:];r[index]=value;rows.append(r)
 data=b'ABONU1\0\0'+struct.pack('<I',len(rows))+b''.join(struct.pack('<9I',*(n&0xffffffff for n in r)) for r in rows);source=a.out/'inputs.bin';output=a.out/'outputs.bin';source.write_bytes(data);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 result=subprocess.run([*runner,str(exe),str(source),str(output)],capture_output=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert result.returncode==0,(result.returncode,result.stderr);raw=output.read_bytes();assert len(raw)==len(rows)*8;counts=[0,0,0,0]
 for i,row in enumerate(rows):
  w,h,seed,x,y,rules,climate,items,mode=row;status=0;value=0x5a5a5a5a
  if climate>255 or mode>2:status=1
  elif mode:status=3
  elif w<2 or w>4096 or w&1 or h<1 or h>4096 or x<0 or y<0 or x>=w or y>=h or (x+y)&1:status=1
  else:value=bonus_model(seed,x,y,rules,climate,items)
  got=struct.unpack_from('<2I',raw,i*8);assert got==(status,value),(i,row,got,(status,value));counts[status]+=1
 assert source.read_bytes()==data
 receipt=dict(cases=len(rows),accepted=counts[0],invalid=counts[1],aliased=counts[3],inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(raw).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),nativeExecution='UNRUN',command=cmd);(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='command'}))
if __name__=='__main__':main()
