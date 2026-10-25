#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline fractional movement expenditure against independent integer model."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def digest():
    h=2166136261
    def word(v):
        nonlocal h
        for b in (v&0xffffffff).to_bytes(4,'little'):h=((h^b)*16777619)&0xffffffff
    def random(seed,lo,hi):
        lo,hi=min(lo,hi),max(lo,hi)
        seed=(seed*1664525+1013904223)&0xffffffff
        return seed,((((hi-lo)*(seed&65535))&0xffffffff)>>16)+lo
    for speed in [0,1,2,3,6,255]:
      for spent in [0,1,2,3,6,255]:
       for cost in [0,1,2,3,6,9,100,255,500]:
        for bits in range(512):
         for execute in range(2):
          road=100 if bits&2 else 3;state=0x140|(0x8000 if bits&1 else 0)
          seed=(bits*0x1234567)&0xffffffff;out_spent=spent;outcome=0;drawn=0;roll=0;error=0
          if spent<speed:
            remaining=speed-spent
            roll_needed=(cost>remaining and bool(bits&4) and
              (remaining<road or bool(bits&8) and not bits&16 and not bits&32 and not bits&64)
              and (bool(bits&256) or not bits&128))
            if roll_needed:seed,roll=random(seed,0,cost);drawn=1
            if roll_needed and roll>=remaining+(road if state&0x8000 else 0):
              out_spent=speed;state|=0x8000;outcome=1
            elif not execute:outcome=2
            elif spent+cost>255:error=2
            else:out_spent=spent+cost;state&=~0x8040;outcome=3
          values=[error,speed,out_spent,state,seed,outcome,drawn,roll]
          if error:values=[error,123,124,125,126,2,1,127]
          for v in values:word(v)
    seed=0x12345678
    for _ in range(6):
      for lo in [0,10,20,7,0xffffffff]:
        seed,value=random(seed,lo,0 if lo==0xffffffff else lo+100)
        word(value);word(seed)
    return f'{h:08x}'

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
    src=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
    if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    exe=out/'move-budget-control';command=[a.cc,*flags,str(src/'move_budget.cpp'),str(src/'move_budget_control.cpp'),'-o',str(exe)]
    subprocess.run(command,check=True)
    r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
    actual=dict(v.split('=') for v in r.stdout.split());expected=dict(budget='331776',rng='30',invalid='5',digest=digest(),rc='0')
    assert actual==expected,(actual,expected)
    (out/'receipt.json').write_text(json.dumps(dict(command=command,runner=runner,results=actual,binarySHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()),indent=2)+'\n');print(r.stdout,end='')
if __name__=='__main__':main()
