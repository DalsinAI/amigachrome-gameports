# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os
import test_local_unit_skip as skip
HERE=Path(__file__).resolve().parent
SOURCES=skip.SOURCES[:-1]+['world_unit_readiness.cpp','local_readiness.cpp','ready_search.cpp','local_ready_search.cpp','ready_search_probe.cpp']
def fixture(version,case):
 text,data,_,_=skip.fixture(version,0);b=bytearray(data);info,o,_=skip.resumed.workers.e.population.decode(b);count=int(info['vehicles']);pf=o['legacy'] if version<11 else o['preferences'];flat=bool(case&8);struct.pack_into('<I',b,o['metadata']+28,int(flat));scenario=case%8;current=0 if case&1 else count-1;reference=current if case&2 else -1;facts=(reference,0 if case&4 else 8,2,current)
 for i in range(count):
  p=o['vehicles']+52*i;x=2*(i%8);y=2+2*(i//8);struct.pack_into('<hh',b,p,x,y);struct.pack_into('<h',b,p+10,64);b[p+14]=2 if scenario==4 or (case&16 and i%3==1) else 1;b[p+16]=0;b[p+36]=3;b[p+17]=1 if scenario==6 else 0;struct.pack_into('<h',b,p+20,-1);struct.pack_into('<hhh',b,p+46,-1,-1,-1);b[p+40]=1 if scenario in [3,4] or (scenario==2 and i!=current) else 0
  state=2 if scenario in [1,2,3,4] or scenario==5 and i==current else 0;struct.pack_into('<I',b,p+4,state)
 return text,bytes(b),facts

def model(text,data,facts):
 normalized=skip.resumed.normalized(text,data);info,o,sp=skip.resumed.workers.e.population.decode(normalized);n=int(info['vehicles']);width=int(info['width']);flat=bool(struct.unpack_from('<I',sp['metadata'],28)[0]);reference,x,y,current=facts;vs=[sp['vehicles'][i*52:(i+1)*52] for i in range(n)];local=reference>=0 and vs[reference][14]==1
 if local:x,y=struct.unpack_from('<hh',vs[reference])
 clear=set();selected=-1;best=9999
 for p in range(3):
  candidates=[]
  for i,v in enumerate(vs):
   state=struct.unpack_from('<I',v,4)[0];ready=v[14]==1 and not(v[17] in [1,2,3] and not state&0x4200) and v[40]<1
   if not ready or state&2 and i not in clear:continue
   tx,ty=struct.unpack_from('<hh',v);dx=abs(x-tx);dy=abs(y-ty)
   if not flat:dx=min(dx,width-dx)
   distance=max(dx,dy)-(((dx+dy)//2-min(dx,dy)+1)//2);candidates.append((distance,0 if i==current else 1,i))
  if candidates:best,_,selected=min(candidates);break
  clear.update(i for i,v in enumerate(vs) if struct.unpack_from('<I',v,4)[0]&2 and (p or i!=current or not local))
 bitmap=bytearray(256);expected=bytearray(normalized)
 for i in clear:bitmap[i//8]|=1<<(i%8);at=o['vehicles']+i*52+4;struct.pack_into('<I',expected,at,struct.unpack_from('<I',expected,at)[0]&~2)
 return selected,best,p+1,bytes(bitmap),bytes(expected)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in range(24):
   text,data,facts=fixture(version,case);selected,distance,passes,bitmap,expected=model(text,data,facts);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';fp=out/f'{i}.facts';rp.write_bytes(text);sp.write_bytes(data);fp.write_bytes(struct.pack('<4i',*facts));r=subprocess.run([*runner,str(exe),str(rp),str(sp),str(fp)],capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);(out/f'{i}.expected').write_bytes(expected);assert r.returncode==0 and struct.unpack_from('<iiI',r.stdout,4)==(selected,distance,passes) and r.stdout[16:272]==bitmap and r.stdout[272:]==expected,(version,case,r.returncode,r.stderr,r.stdout[:16],selected,distance,passes,next((j for j,(x,y) in enumerate(zip(r.stdout[272:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,selected=selected,passes=passes,cleared=sum(x.bit_count() for x in bitmap),guards=struct.unpack_from('<I',r.stdout)[0]))
 receipt=dict(worlds=len(records),selected=sum(r['selected']>=0 for r in records),none=sum(r['selected']<0 for r in records),cleared=sum(r['cleared'] for r in records),guards=sum(r['guards'] for r in records),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
