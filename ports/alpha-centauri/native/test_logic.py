#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned social/nutrient arithmetic vs pinned source and independent Python digest."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def reference_digest():
    digest=2166136261
    def add(values):
        nonlocal digest
        for value in values:
            for byte in (value & 0xffffffff).to_bytes(4,'little'):
                digest=((digest^byte)*16777619)&0xffffffff
    table=[[[((c*11+m*3+e)%9)-4 for e in range(11)] for m in range(4)] for c in range(4)]
    for choice in range(256):
        selected=[(choice>>(c*2))&3 for c in range(4)]
        for flags in range(32):
            for quick in range(2):
                values=[0]*11
                for c,m in enumerate(selected):
                    for e,v in enumerate(table[c][m]):
                        if v<0 and ((c==3 and ((m==1 and flags&1) or (m==3 and flags&2))) or (c==2 and m==1 and flags&2)):
                            v=0
                        if v<0:
                            if c==0 and m==2:
                                values[e]-=v
                            if c==1 and m==3:
                                values[e]+=v
                        values[e]+=v
                if not quick:
                    values[5]+=bool(flags&4)
                    values[2]+=2*bool(flags&8)
                    values[7]+=bool(flags&16)
                    values[10]+=bool(flags&16)*bool(choice&1)
                    values[6]+=2;values[0]-=1;values[10]-=3
                    if values[2]<0:
                        values[2]=-((-values[2])//2)
                    values[1]=max(0,min(999,values[1]))
                    if values[7]<0:
                        values[7]=-((-values[7])//2)
                add(values)
    seed=0x12345678
    for i in range(2001):
        if i<2000:
            seed=(seed*1664525+1013904223)&0xffffffff
            population,requirement,intake,accumulated=seed%10,2,seed%31,seed%20-5
            incoming,outgoing,growth,stage,total=seed%5,seed%3,seed%7-3,seed%3,seed%101
            creche,golden=bool(seed&32),bool(seed&64)
        else:
            population,requirement,intake,accumulated=2,2,200,0
            incoming,outgoing,growth,stage,total=3,1,1,1,100
            creche=golden=True
        growth+=2*creche+2*golden
        intake+=incoming
        consumption=outgoing+population*requirement
        surplus=intake-consumption
        if surplus>=0 and accumulated<0:
            accumulated=0
        elif surplus<0 and accumulated==0:
            accumulated=-1
        if stage==1:
            total+=max(0,min(99,surplus))
        add([growth,intake,consumption,surplus,accumulated,total])
    return f'{digest:08x}'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--cc',default='c++')
    ap.add_argument('--m68k-linux',action='store_true')
    args=ap.parse_args()
    source=Path(__file__).resolve().parent
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=False)
    executable=out/'logic-control'
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']
    runner=[]
    if args.m68k_linux:
        flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=[args.cc,*flags,str(source/'game_logic.cpp'),str(source/'logic_control.cpp'),'-o',str(executable)]
    subprocess.run(command,check=True)
    result=subprocess.run([*runner,str(executable)],capture_output=True,text=True,timeout=60,
                          env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    assert result.returncode==0,(result.returncode,result.stderr,result.stdout)
    actual=dict(word.split('=') for word in result.stdout.split())
    expected=dict(social='16384',nutrients='2001',invalid='13',digest=reference_digest(),rc='0')
    assert actual==expected,(actual,expected)
    receipt=dict(command=command,runner=runner,results=actual,
                 programSHA256=hashlib.file_digest(executable.open('rb'),'sha256').hexdigest(),
                 reference='Pinned upstream function bodies with explicit platform/layout adapters, plus independent Python digest',
                 scope='Social modifiers and one base nutrient calculation only; not an integrated turn, map, AI or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(result.stdout,end='')


if __name__=='__main__':
    main()
