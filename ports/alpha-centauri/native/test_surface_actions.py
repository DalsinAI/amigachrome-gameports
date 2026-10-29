#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Sea capability/refusal boundary and independent complete-container oracle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import zlib
from test_local_actions import SOURCES, rule_data, local_world
from test_world_actions import fixture as land_fixture
from test_original_game import decode
from test_units import sections, material

HERE = Path(__file__).resolve().parent
DIRECTIONS = [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]

def stacks(data):
    b=bytearray(data);info,o,_=decode(b);width=int(info['width']);groups={}
    for i in range(int(info['width'])*int(info['height'])//2):
        p=o['tiles']+44*i;struct.pack_into('<I',b,p+8,struct.unpack_from('<I',b,p+8)[0]&~2)
    for i in range(int(info['vehicles'])):
        x,y=struct.unpack_from('<hh',b,o['vehicles']+i*52);groups.setdefault((x,y),[]).append(i)
    for (x,y),group in groups.items():
        p=o['tiles']+((y*width+x)//2)*44;struct.pack_into('<I',b,p+8,struct.unpack_from('<I',b,p+8)[0]|2)
        b[p+2]=(b[p+2]&0xf0)|b[o['vehicles']+group[0]*52+14]
        for n,i in enumerate(group):struct.pack_into('<hh',b,o['vehicles']+52*i+48,group[n+1] if n+1<len(group) else -1,group[n-1] if n else -1)
    return bytes(b)

def fixture(version=7,vehicles=1,bases=0):
    b=bytearray(local_world(land_fixture(version,vehicles,bases)));info,o,_=decode(b)
    for i in range(int(info['width'])*int(info['height'])//2):
        p=o['tiles']+i*44;b[p]=0x40
    p=o['prototypes']+320*52;b[p+36]=3;b[p+37]=16;b[p+40]=2;b[p+42]=7
    for i in range(bases):
        p=o['bases']+i*(308 if version>=11 else 304);struct.pack_into('<hh',b,p,3+2*i,3);b[p+4]=5
        t=o['tiles']+((3*int(info['width'])+3+2*i)//2)*44;b[t]=0x60;b[t+7]=5
        struct.pack_into('<I',b,t+8,struct.unpack_from('<I',b,t+8)[0]|1)
    return stacks(b)

def relocate(data,vehicle,x,y,price):
    """Independent wire oracle for recovered veh_lift/drop and full-cost spend."""
    b=bytearray(data);info,o,_=decode(b);vp=o['vehicles'];v=vp+52*vehicle;width=int(info['width'])
    sx,sy=struct.unpack_from('<hh',b,v);next_,prev=struct.unpack_from('<hh',b,v+48)
    if prev>=0:struct.pack_into('<h',b,vp+52*prev+48,next_)
    if next_>=0:struct.pack_into('<h',b,vp+52*next_+50,prev)
    source=o['tiles']+((sy*width+sx)//2)*44;dest=o['tiles']+((y*width+x)//2)*44
    if next_<0 and prev<0:struct.pack_into('<I',b,source+8,struct.unpack_from('<I',b,source+8)[0]&~2)
    top=-1
    for i in range(int(info['vehicles'])):
        if i!=vehicle and struct.unpack_from('<hh',b,vp+i*52)==(x,y) and struct.unpack_from('<h',b,vp+i*52+50)[0]<0:top=i;break
    struct.pack_into('<hh',b,v,x,y);struct.pack_into('<hh',b,v+48,top,-1)
    if top>=0:struct.pack_into('<h',b,vp+52*top+50,vehicle)
    bits=struct.unpack_from('<I',b,dest+8)[0]
    if top<0 and not(bits&1):b[dest+2]=(b[dest+2]&0xf0)|b[v+14]
    struct.pack_into('<I',b,dest+8,bits|2|0x100000)
    struct.pack_into('<I',b,v+4,(struct.unpack_from('<I',b,v+4)[0]|4)&~0x8040);b[v+40]+=price
    direction=next(i for i,(dx,dy) in enumerate(DIRECTIONS) if (sx+dx)%width==x and sy+dy==y);b[v+41]=direction
    return bytes(b)

def checkpoint(payload,rules):
    tail=struct.pack('<IIII',len(payload),zlib.crc32(payload),0xabcdef01,zlib.crc32(rules))+payload
    return b'ACLOCAL1'+struct.pack('<I',zlib.crc32(tail))+tail

def corpus():
    baseline=fixture();info,o,_=decode(baseline);v=o['vehicles'];p=o['prototypes']+320*52
    ti=lambda x,y:o['tiles']+((y*int(info['width'])+x)//2)*44
    result=[]
    def case(name,data=baseline,status='moved',price=3,route=0,x=3,y=3,id=0):result.append(dict(name=name,data=data,status=status,price=price,route=route,x=x,y=y,id=id))
    def mutate(name,offset,fmt,value,status='unsupported',price=0):
        b=bytearray(baseline);struct.pack_into('<'+fmt,b,offset,value);case(name,bytes(b),status,price)
    for version in range(7,11):case('base-version-'+str(version),fixture(version))
    for i,(dx,dy) in enumerate(DIRECTIONS):case('direction-'+str(i),x=2+dx,y=2+dy)
    case('land-only-refuses-sea',route=1,status='unsupported',price=0)
    case('bad-road',route=2,status='bad_state',price=0)
    case('player-checkpoint-roundtrip',route=10)
    case('player-two-moves-exhausted',route=11,status='exhausted')
    case('explicit-capability-session-authority',route=12)
    case('not-adjacent',x=6,y=4,status='bad_state',price=0)
    case('bad-parity',x=2,y=3,status='bad_state',price=0)
    case('bad-vehicle',id=999,status='bad_state',price=0)
    mutate('not-owner',v+14,'B',6,'not_owner')
    mutate('human-bit-required',o['legacy']+0x58,'B',0)
    mutate('active-faction-required',o['legacy']+0x390-0x7c,'i',6)
    for bit in [1,0x200,0x4000,0x1000000,0x2000000,0x8000000]:mutate('waking-state-'+hex(bit),v+4,'I',bit)
    for off in [17,18]:mutate('standing-order-'+str(off),v+off,'B',1)
    mutate('ability',p+32,'I',0x400)
    mutate('psi-design',p+37,'B',12)
    mutate('unsupported-plan',p+42,'b',9)
    mutate('zero-capacity-transport',p+40,'B',0)
    mutate('capacity-is-not-passengers',p+40,'B',99,'moved',3)
    mutate('exhausted',v+40,'B',255,'exhausted',3)
    mutate('insufficient-full-cost',v+40,'B',5,'unsupported',3)
    mutate('spent-5-exhausted-after-shorter-budget',v+40,'B',6,'exhausted',3)
    mutate('preserve-working-state',v+4,'I',0x4000000,'moved',3)
    mutate('clear-fractional-marker',v+4,'I',0x8000,'moved',3)
    mutate('foreign-territory',ti(3,3)+7,'b',6)
    mutate('source-foreign-territory',ti(2,2)+7,'b',6)
    mutate('fog-destination',ti(3,3)+4,'B',0)
    mutate('fog-neighborhood',ti(4,4)+4,'B',0)
    mutate('plain-shore-not-base',ti(3,3),'B',0x60)
    mutate('orphan-base-bit',ti(3,3)+8,'I',0x100001,'bad_state')
    for off in [ti(2,2),ti(3,3)]:
        for bit in [0x20,0x2000]:mutate('encounter-'+str(off)+'-'+str(bit),off+8,'I',struct.unpack_from('<I',baseline,off+8)[0]|bit)
    mutate('explicit-pod',ti(3,3)+8,'I',0x10000000)
    for kind in ['combat','naval-stack','open-sea-unmarked-land','remote-boarder','owned-base-arrival-garrison','owned-base-departure-garrison','owned-base-unassigned-boarder','owned-base-linked-boarder','owned-base-loaded','foreign-neighbor','foreign-far']:
        b=bytearray(fixture(vehicles=2,bases=int(kind.startswith('owned-base'))));_,off,_=decode(b);v2=off['vehicles']+52;q=off['prototypes']+321*52
        if kind=='combat':
            b[off['prototypes']+320*52+37]=0;b[off['prototypes']+320*52+40]=0;b[off['prototypes']+320*52+42]=1
        elif kind=='naval-stack':struct.pack_into('<hh',b,v2,3,3)
        elif kind=='foreign-neighbor':b[v2+14]=6
        elif kind=='foreign-far':struct.pack_into('<hh',b,v2,6,4);b[v2+14]=6
        else:
            b[q:q+52]=bytes(52);b[q+39]=1;b[q+42]=2;struct.pack_into('<h',b,v2+10,321)
            if kind.startswith('owned-base'):
                if kind.startswith('owned-base-departure') or kind not in ['owned-base-arrival-garrison']:
                    bp=off['bases'];struct.pack_into('<hh',b,bp,2,2)
                    dest=off['tiles']+((3*8+3)//2)*44;source=off['tiles']+((2*8+2)//2)*44
                    struct.pack_into('<I',b,dest+8,0x100000);b[dest]=0x40;b[dest+7]=0xff;struct.pack_into('<I',b,source+8,0x100001);b[source]=0x60;b[source+7]=5
                    struct.pack_into('<hh',b,v2,2,2)
                else:struct.pack_into('<hh',b,v2,3,3)
                b[v2+17]=2
                if 'boarder' in kind:b[v2+17]=1;struct.pack_into('<h',b,v2+20,-1 if 'unassigned' in kind else 0)
                if 'loaded' in kind:struct.pack_into('<I',b,v2+4,1)
            elif kind=='remote-boarder':b[v2+17]=1;struct.pack_into('<h',b,v2+20,0)
            else:struct.pack_into('<hh',b,v2,2,2)
        status='unsupported' if kind in ['open-sea-unmarked-land','remote-boarder','owned-base-unassigned-boarder','owned-base-linked-boarder','owned-base-loaded','foreign-neighbor'] else 'moved'
        case(kind,stacks(b),status,0 if status=='unsupported' else 3)
    return result

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--binary',type=Path);ap.add_argument('--original',type=Path);ap.add_argument('--original-rules',type=Path);args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=False)
    binary=args.binary or args.out/'surface-actions-probe';command=[]
    if not args.binary:
        files=[s for s in SOURCES if s!='local_actions_probe.cpp']+['local_player.cpp','surface_actions_probe.cpp'];command=[args.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/f)for f in files],'-o',str(binary)];subprocess.run(command,check=True)
    tables=sections(rule_data());tables['CHASSIS'][3][8]='3';tables['CHASSIS'][3][9]='1';tables['CHASSIS'][3][12]='2';tables['WEAPONS'][16][2]='0';tables['WEAPONS'][16][3]='7';tables['WEAPONS'][0][2]='1';tables['WEAPONS'][0][3]='0';rules=material(tables);cases=corpus()
    if args.original:
        assert args.original_rules
        for route in [0,1,10]:cases.append(dict(name='genuine-empty-foil-'+str(route),data=args.original.read_bytes(),rules=args.original_rules.read_bytes(),status='unsupported' if route==1 else 'moved',price=0 if route==1 else 3,route=route,x=19,y=35,id=19))
    records=[];combined=b''
    for i,c in enumerate(cases):
        data=c['data'];text=c.get('rules',rules);rp=args.out/f'{i}.txt';sp=args.out/f'{i}.SC';out=args.out/f'{i}.after';rp.write_bytes(text);sp.write_bytes(data)
        run=subprocess.run([str(binary),str(rp),str(sp),str(c['id']),str(c['x']),str(c['y']),str(c['route']),str(out)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=120);assert run.returncode==0,(c['name'],run.returncode,run.stdout,run.stderr)
        values=dict(part.split('=',1) for part in run.stdout.split() if '='in part);assert values['order']=='ok' and values['status']==c['status'] and int(values['price'])==c['price'],(c['name'],values,c['status'],c['price'])
        expected=data
        if c['status']=='moved':expected=relocate(data,c['id'],c['x'],c['y'],c['price'])
        if c['route']==11:
            info,off,_=decode(data);sx,sy=struct.unpack_from('<hh',data,off['vehicles']+52*c['id']);expected=relocate(relocate(data,c['id'],c['x'],c['y'],3),c['id'],sx,sy,3)
        if c['route']in [10,11]:expected=checkpoint(expected,text)
        actual=out.read_bytes();assert actual==expected,(c['name'],'complete-container mismatch',[(j,a,b)for j,(a,b)in enumerate(zip(actual,expected))if a!=b][:20]);assert sp.read_bytes()==data and rp.read_bytes()==text
        (args.out/f'{i}.expected').write_bytes(expected);(args.out/f'{i}.summary').write_text(run.stdout);combined+=actual
        records.append({k:v for k,v in c.items()if k not in ['data','rules']}|dict(index=i,bytes=len(actual),sha256=hashlib.sha256(actual).hexdigest(),checks=int(values['checks']),passed=True))
    receipt=dict(cases=len(records),records=records,aggregateBytes=len(combined),aggregateSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256(binary.read_bytes()).hexdigest(),command=command,native='UNRUN');(args.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items()if k not in ['records','command']}))
if __name__=='__main__':main()
