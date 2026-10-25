#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Predefined prototype arithmetic vs pinned source and independent Python."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from test_units import reference as unit_reference


def decode(data,variant):
    fields=unit_reference(data,variant)
    result={'chassis':[],'weapons':[],'armor':[],'abilities':[],'units':[]}
    for group,count,keys in [('chassis',9,['speed','triad','missile','cargo','cost']),
        ('weapons',int(fields['weapon_count']),['offense','mode','cost']),
        ('armor',int(fields['armor_count']),['defense','cost']),
        ('abilities',int(fields['ability_count']),['cost_factor','flags']),
        ('units',int(fields['unit_count']),['chassis','weapon','armor','reactor','plan','cost','carry','prerequisite','icon','abilities'])]:
        for i in range(count):
            row={key:int(fields[f'{group}.{i}.{key}']) for key in keys}
            if group=='units':row['name']=fields[f'units.{i}.name']
            result[group].append(row)
    return result


def cost(d,c,w,a,mask,reactor):
    ch,wp,ar=d['chassis'][c],d['weapons'][w],d['armor'][a]
    if ch['missile'] and wp['offense']>=99:return wp['cost']
    wc,ac,sc=wp['cost'],ar['cost'],ch['cost']
    modifier=flags=0
    for i,ability in enumerate(d['abilities']):
        if mask&(1<<i):
            if modifier:modifier+=1
            factor=ability['cost_factor']
            if factor>0:modifier+=factor
            elif factor==-1:modifier+=min(2,wc//ac) if ac else 0
            elif factor in [-2,-3,-4]:modifier += {-2:wc,-3:ac,-4:sc}[factor]-1
            elif factor in [-5,-6,-7]:modifier += {-5:wc+ac,-6:wc+sc,-7:ac+sc}[factor]-2
            if ability['flags']&0x400 and ch['triad']==0:flags+=1
    modifier+=flags
    if ch['triad']==1:ac//=2;sc+=reactor
    elif ch['triad']==2:
        if ac>1:ac*=reactor*2
        sc+=reactor*2
    combat=max(ac//2+1,wc)
    if combat==ac==sc==reactor==1:proto=1
    else:
        proto=((sc+ac)*combat+(1<<reactor))//(2<<reactor)
        if sc==1:proto=proto//2+1
        if wc>1 and ar['cost']>1:
            proto+=1
            if ch['triad']==0 and sc>1:proto+=1
        if ch['triad']==1 and wp['mode']!=11:proto=(proto+1)//2
        elif ch['triad']==2:proto//=2 if wp['mode']>2 else 4
        proto=max(proto,(reactor*3+1)//2)
    return (proto*(modifier+4)+2)//4


def derive(d,index,road):
    b=d['units'][index];c=d['chassis'][b['chassis']];w=d['weapons'][b['weapon']];a=d['armor'][b['armor']]
    abilities=b['abilities'];reactor=b['reactor']
    price=cost(d,b['chassis'],b['weapon'],b['armor'],abilities,reactor)
    carry=0
    if w['mode']==7:
        carry=c['cargo']*(reactor if c['triad']==1 else 1)
        if abilities&0x800000:carry//=2
        if abilities&0x80000:carry=(carry*3+1)//2
    if c['missile']:
        plan,role=(5,14) if w['offense']>=99 else (13,22) if b['weapon']==24 else (14,23) if b['weapon']==25 else (0,13)
    elif w['mode']>=7:plan,role=w['mode'],w['mode']+32
    elif c['triad']==1:plan,role=6,6
    elif c['triad']==2:plan,role=(4,9) if abilities&0x20 else (0,8)
    else:
        plan=2 if a['defense']>1 else 0
        if w['offense']>=a['defense'] and plan==2:plan=1
        if c['speed']>1 and w['offense']>a['defense']:plan=0
        if abilities&0x8018:plan=0
        role=2 if a['defense']>w['offense'] else 3
        if w['offense']>a['defense']*2:role=1
        if c['speed']==2:role=4
        elif c['speed']>=3:role=5
        if w['offense']==a['defense']==1:
            if c['speed']>1:plan=3
            if abilities&0x400000:plan=2
            role=10
    if b['plan']!=-1:plan=b['plan']
    if b['cost']:price=b['cost']
    if b['carry']:carry=b['carry']
    if price>255 or carry>255:return None
    if index==19:speed=0
    else:
        speed=c['speed']+(reactor*2 if c['triad']==2 else 0)
        if w['mode']==7:speed-=1
        if abilities&0x800000:speed-=1
        if abilities&0x400:speed+=reactor*2 if c['triad']==2 else 1
        if c['triad']==2:
            if abilities&0x8000000:speed+=2
            if abilities&0x20:speed=speed*4//5
            if w['mode']==7:speed//=2
        speed=min(99,max(1,speed))*road
    return [b['chassis'],b['weapon'],b['armor'],reactor,b['prerequisite'],abilities,
            price,carry,plan,role,b['icon'],0,-1,1,speed]


def fixture(expansion):
    wc,ac,bc=(26,14,29) if expansion else (21,10,24)
    speeds=[1,2,3,4,6,8,8,8,12]
    d={'chassis':[dict(speed=s,cost=s,cargo=2,triad=0 if i<3 else 1 if i<5 else 2,missile=int(i==8)) for i,s in enumerate(speeds)],
       'weapons':[dict(offense=i+1,mode=i%3,cost=i%10+1) for i in range(wc)],
       'armor':[dict(defense=i+1,cost=i%10+1) for i in range(ac)],
       'abilities':[dict(cost_factor=i-7,flags=0x400 if i%2 else 0) for i in range(bc)],'units':[]}
    d['weapons'][1]['offense']=99
    for i,mode in enumerate([7,8,9,10,11,12],2):d['weapons'][i]['mode']=mode
    d['units']=[dict(name='U'+chr(65+i%26),chassis=i%9,weapon=i%wc,armor=i%ac,reactor=i%4+1,
        plan=-1,prerequisite=-1,icon=-1,cost=0,carry=0,abilities=1<<(i%bc)) for i in range(64)]
    return d


def reference_digest():
    h=2166136261
    def words(values):
        nonlocal h
        for value in values:
            for byte in (value&0xffffffff).to_bytes(4,'little'):h=((h^byte)*16777619)&0xffffffff
    for variant in range(2):
        d=fixture(bool(variant));bc=len(d['abilities'])
        for c in range(9):
            for reactor in range(1,5):
                for combo in range(32):
                    mask=(1<<(combo%bc))|(1<<((combo*3)%bc)) if combo else 0
                    words([cost(d,c,combo*7%len(d['weapons']),combo*3%len(d['armor']),mask,reactor)])
        for i in range(64):
            for rate in [1,3,10,100]:words(derive(d,i,rate) or [0xffffffff])
        d['units'][0].update(cost=7,carry=3,plan=12,icon=5)
        words(derive(d,0,3))
    return f'{h:08x}'


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');args=ap.parse_args()
    src=Path(__file__).resolve().parent;out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'prototype-control'
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
    if args.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=[args.cc,*flags,str(src/'prototype.cpp'),str(src/'prototype_control.cpp'),'-o',str(exe)];subprocess.run(command,check=True)
    r=subprocess.run([*runner,str(exe)],capture_output=True,text=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
    actual=dict(item.split('=') for item in r.stdout.split());expected=dict(cost='2304',derived='514',invalid='6',digest=reference_digest(),rc='0');assert actual==expected,(actual,expected)
    receipt=dict(command=command,runner=runner,results=actual,programSHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest(),scope='Predefined prototype pricing/transport/initial stats/speed only; no designer/history/unit movement or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(r.stdout,end='')


if __name__=='__main__':main()
