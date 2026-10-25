#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Map geometry and predefined adjacent movement pricing vs source/Python."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from test_prototype import fixture,derive


def items(mask):
    return sum(v for i,v in enumerate([4,8,0x20,0x80,0x200000,1]) if mask&(1<<i))


def movement_cost(d,index,pattern,context,road):
    p=derive(d,index,road);assert p is not None
    source=items(pattern);dest=items((pattern*13+7)&63)
    src_ocean=pattern%7==0
    alt=[0x60,0x40,0x20][pattern%3]
    rock=[0,0x40,0x80][pattern//3%3]
    faction=0 if context&8 else 1
    planet=int(bool(context&4));xeno=bool(context&1);ignore=bool(context&2)
    c=d['chassis'][p[0]];w=d['weapons'][p[1]]
    if alt<0x60:
        return road*3 if dest&0x20 and alt==0x40 and c['triad']==1 and index not in [14,9] and not xeno else road
    if src_ocean or c['triad']!=0:return road
    if source&9 and dest&9 and faction:return 0
    if (source&5 or source&0x20 and faction>0 and xeno) and dest&5 and faction:return 1
    if (xeno or faction==0 or index in [8,15]) and dest&0x20:return 1
    if source&0x80 and dest&0x80 and faction:return 1  # matrix always uses adjacent diagonals, including wrapped seam
    if p[0]==2 or p[5]&0x400:return road
    value=road+(road if rock>0x40 and not ignore else 0)+(road if dest&0x200000 and not ignore else 0)
    if faction and dest&0x20 and w['offense']>=0:
        if p[8] not in [9,12] and planet<=0:return value+2*road
        if value<=p[14]:return p[14]
    return value


def definitions():
    d=fixture(True)
    for row in d['units']:row.update(cost=1,carry=1)
    d['weapons'][8]['offense']=-1
    for i in [8,15,19]:d['units'][i].update(chassis=0,weapon=8,abilities=0)
    for i in [9,14]:d['units'][i].update(chassis=3,weapon=8,abilities=0)
    d['units'][1].update(chassis=0,weapon=4,plan=9,abilities=0)
    d['units'][7].update(chassis=0,weapon=7,plan=12,abilities=0)
    return d


def reference_digest():
    h=2166136261
    def word(value):
        nonlocal h
        for byte in (value&0xffffffff).to_bytes(4,'little'):h=((h^byte)*16777619)&0xffffffff
    d=definitions()
    for i in [0,1,2,3,7,8,9,10,11,14,15,19]:
        for pattern in range(64):
            for ctx in range(16):
                for road in [3,100]:word(movement_cost(d,i,pattern,ctx,road))
    for expected in [1,1,3,3,0,3,1,3,9,3,3,3,9,1,1,1,1,3,0,9]:word(expected)
    for width in [2,4,8,80,32766]:
        for height in [1,8,40,32767]:
            for flat in [0,1]:
                xs=[0,1,width-2,width-1]
                for x in xs:
                    for y in [0,height-1]:word(0xffffffff if (x+y)&1 else (x+width*y)//2)
                for x in [-width,-1,0,width-1,width,width*2]:word(x if flat else x+width if x<0 else x-width if x>=width else x)
                for a in xs:
                    for b in xs:
                        distance=abs(a-b)
                        word(width-distance if not flat and distance>width//2 else distance)
    return f'{h:08x}'


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args()
    src=Path(__file__).resolve().parent;out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'movement-control'
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
    if args.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=[args.cc,*flags,str(src/'prototype.cpp'),str(src/'movement.cpp'),str(src/'movement_control.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
    r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
    actual=dict(item.split('=') for item in r.stdout.split());expected=dict(movement='24576',named='20',geometry='1200',invalid='14',digest=reference_digest(),rc='0');assert actual==expected,(actual,expected)
    receipt=dict(command=command,runner=runner,results=actual,programSHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest(),scope='Adjacent predefined step pricing and coordinate geometry only; not access/occupancy/combat/ZOC/action dispatch or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(r.stdout,end='')


if __name__=='__main__':main()
