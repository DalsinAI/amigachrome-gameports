#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native component/blueprint parser controls; optional originals remain private."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from test_faction import number

SECTIONS=['CHASSIS','REACTORS','WEAPONS','DEFENSES','ABILITIES','UNITS']


def sections(data):
    result={};name=None
    for line in data.decode('cp1252').splitlines():
        line=line.split(';',1)[0].strip()
        if line.startswith('#'):
            name=line[1:].upper();result[name]=[]
        elif line and name:
            result[name].append([v.strip() for v in line.split(',')])
    return result


def reference(data,variant):
    tables=sections(data)
    tech={row[1].lower():i for i,row in enumerate(tables['TECHNOLOGY'])}
    tech.update(none=-1,disable=-2)
    wc,ac,bc=(21,10,24) if variant=='base' else (26,14,29)
    total=number(tables['UNITS'][0][0])
    out={'weapon_count':str(wc),'armor_count':str(ac),'ability_count':str(bc),'unit_count':str(total)}
    def put(prefix,key,value):out[prefix+'.'+key]=str(value)
    chassis={};weapons={};armor={}
    assert len(tables['CHASSIS'])==9 and len(tables['REACTORS'])==4
    assert len(tables['WEAPONS'])==wc and len(tables['DEFENSES'])==ac and len(tables['ABILITIES'])==bc
    assert len(tables['UNITS'])==total+1
    for i,row in enumerate(tables['CHASSIS']):
        prefix=f'chassis.{i}';chassis[row[0].lower()]=i
        for j,pos in enumerate([0,2,4,6,15,17]):
            grammar=row[pos+1]
            put(prefix,f'noun.{j}.name',row[pos])
            put(prefix,f'noun.{j}.gender',{'f':1,'n':2}.get(grammar[0].lower(),0))
            put(prefix,f'noun.{j}.plural',int(len(grammar)>1 and grammar[1]=='2'))
        for key,token in zip(['speed','triad','range','missile','cargo','cost'],row[8:14]):put(prefix,key,number(token))
        put(prefix,'prerequisite',tech[row[14].lower()])
    for i,row in enumerate(tables['REACTORS']):
        prefix=f'reactors.{i}';put(prefix,'name',row[0]);put(prefix,'short_name',row[1]);put(prefix,'power',number(row[2]));put(prefix,'prerequisite',tech[row[3].lower()])
    for i,row in enumerate(tables['WEAPONS']):
        prefix=f'weapons.{i}';weapons[row[1].lower()]=i
        put(prefix,'name',row[0]);put(prefix,'short_name',row[1])
        for key,token in zip(['offense','mode','cost','icon'],row[2:6]):put(prefix,key,number(token))
        put(prefix,'prerequisite',tech[row[6].lower()])
    for i,row in enumerate(tables['DEFENSES']):
        prefix=f'armor.{i}';armor[row[1].lower()]=i
        put(prefix,'name',row[0]);put(prefix,'short_name',row[1])
        for key,token in zip(['defense','mode','cost'],row[2:5]):put(prefix,key,number(token))
        put(prefix,'prerequisite',tech[row[5].lower()])
    for i,row in enumerate(tables['ABILITIES']):
        prefix=f'abilities.{i}'
        put(prefix,'name',row[0]);put(prefix,'cost_factor',number(row[1]));put(prefix,'prerequisite',tech[row[2].lower()])
        put(prefix,'abbreviation',row[3]);put(prefix,'flags',int(row[4],2));put(prefix,'description',row[5])
    for i,row in enumerate(tables['UNITS'][1:]):
        prefix=f'units.{i}';put(prefix,'name',row[0]);put(prefix,'chassis',chassis[row[1].lower()]);put(prefix,'weapon',weapons[row[2].lower()]);put(prefix,'armor',armor[row[3].lower()])
        for key,token in zip(['plan','cost','carry'],row[4:7]):put(prefix,key,number(token))
        put(prefix,'prerequisite',tech[row[7].lower()]);put(prefix,'icon',number(row[8]));put(prefix,'abilities',int(row[9],2))
        reactor=number(row[10]) if len(row)>10 and row[10] else 0
        put(prefix,'reactor',reactor or (2 if i==17 else 3 if i==18 else 1))
    return out


def fixture(variant='expansion',count=20):
    wc,ac,bc=(21,10,24) if variant=='base' else (26,14,29)
    t={'TECHNOLOGY':[[f'Test {i}',f'T{i}','0','1','2','3','None','None','1'] for i in range(89)]}
    t['CHASSIS']=[[f'C{i}','M1',f'Off{i}','F2',f'Def{i}','N1',f'Guard{i}','M2',str(i+1),str(i%3),'2',str(i==8 and 1 or 0),'1',str(i+2),'None',f'Large{i}','F1',f'Big{i}','N2'] for i in range(9)]
    t['REACTORS']=[[f'Reactor{i}',f'R{i}',str(i+1),'T88'] for i in range(4)]
    t['WEAPONS']=[[f'Weapon{i}',f'W{i}',str(i-1),str(i%13),str(i+1),str(i-1),'None'] for i in range(wc)]
    t['DEFENSES']=[[f'Armor{i}',f'A{i}',str(i-1),str(i%3),str(i+1),'Disable'] for i in range(ac)]
    t['ABILITIES']=[[f'Ability{i}',str(i-7),'T0','' if i==0 else f'Ab{i}','000101',f'Effect {i}'] for i in range(bc)]
    t['UNITS']=[[str(count)]]+[[f'Unit{i}',f'C{i%9}',f'W{i%wc}',f'A{i%ac}','-1','0','0','None','-1','10001'] for i in range(count)]
    return t


def material(t):
    return ('\n'.join('#'+key+'\n'+'\n'.join(','.join(row) for row in rows) for key,rows in t.items())+'\n#END\n').encode('cp1252')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner-dir',type=Path)
    args=ap.parse_args();src=Path(__file__).resolve().parent;out=args.out.resolve();out.mkdir(parents=True,exist_ok=False)
    exe=out/'unit-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
    if args.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=[args.cc,*flags,str(src/'rules_text.cpp'),str(src/'unit_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True);results=[]
    def check(name,data,variant='expansion',error=None):
        path=out/(name+'.txt');path.write_bytes(data)
        r=subprocess.run([*runner,str(exe),variant,str(path)],capture_output=True,text=True,encoding='cp1252',timeout=20,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if error:assert r.returncode==20 and error in r.stderr,(name,r.returncode,r.stderr)
        else:
            assert r.returncode==0,(name,r.returncode,r.stderr)
            actual=dict(line.split('=',1) for line in r.stdout.splitlines());expected=reference(data,variant)
            assert actual==expected,(name,[(k,v,expected.get(k)) for k,v in actual.items() if expected.get(k)!=v])
        results.append(dict(case=name,passed=True,expected_error=error))
    for variant in ['base','expansion']:
        data=material(fixture(variant))
        check(variant,data,variant);check(variant+'-crlf',data.replace(b'\n',b'\r\n'),variant);check(variant+'-cr',data.replace(b'\n',b'\r'),variant)
        check(variant+'-comments',data.replace(b'#CHASSIS\n',b'#chassis\n\n;comment\n'),variant)
    data=material(fixture())
    for count in [0,64]:check('unit-count-'+str(count),material(fixture(count=count)))
    t=fixture();t['UNITS'][1]+=['4'];t['UNITS'][2]+=['0'];check('explicit-reactors',material(t))
    t=fixture();t['ABILITIES'][0][5]='';check('optional-description',material(t))
    t=fixture();t['UNITS'][1][-1]='10000000000000000000000000000000';check('unsigned-mask',material(t))
    check('casefold',data.replace(b'C0,W0,A0',b'c0,w0,a0').replace(b',T88',b',t88'))
    check('terminal-comma',data.replace(b'\n',b',\n').replace(b'#TECHNOLOGY,',b'#TECHNOLOGY').replace(b'#CHASSIS,',b'#CHASSIS').replace(b'#REACTORS,',b'#REACTORS').replace(b'#WEAPONS,',b'#WEAPONS').replace(b'#DEFENSES,',b'#DEFENSES').replace(b'#ABILITIES,',b'#ABILITIES').replace(b'#UNITS,',b'#UNITS').replace(b'#END,',b'#END'))
    check('wrong-base',data,'base',error='bad row');check('wrong-expansion',material(fixture('base')),error='bad row')
    for name in SECTIONS:
        t=fixture();del t[name];check('missing-'+name,material(t),error='missing section')
        rows=fixture()[name]
        for i in range(len(rows)):
            t=fixture();t[name]=rows[:i];check('truncate-'+name+'-'+str(i),material(t),error='bad row')
        t=fixture();t[name].append(t[name][-1][:]);check('extra-'+name,material(t),error='bad row')
    for name,section,row,field,value,error in [
        ('duplicate-chassis','CHASSIS',1,0,'C0','duplicate key'),
        ('duplicate-weapon','WEAPONS',1,1,'W0','duplicate key'),
        ('duplicate-armor','DEFENSES',1,1,'A0','duplicate key'),
        ('unknown-tech','REACTORS',0,3,'Missing','unknown prerequisite'),
        ('unknown-chassis','UNITS',1,1,'Missing','bad key'),
        ('unknown-weapon','UNITS',1,2,'Missing','bad key'),
        ('unknown-armor','UNITS',1,3,'Missing','bad key'),
        ('bad-triad','CHASSIS',0,9,'3','bad number'),
        ('byte-overflow','CHASSIS',0,8,'256','bad number'),
        ('signed-byte-overflow','WEAPONS',0,2,'128','bad number'),
        ('negative-byte','WEAPONS',0,4,'-1','bad number'),
        ('bad-bits','ABILITIES',0,4,'1002','bad number'),
        ('bad-unit-bits','UNITS',1,9,'10x1','bad number'),
        ('unit-over-count','UNITS',0,0,'65','bad number'),
        ('unit-negative-count','UNITS',0,0,'-1','bad number'),
        ('overflow','ABILITIES',0,1,'2147483648','bad number'),
        ('empty-component','UNITS',1,1,'','bad row'),
        ('long-unit-name','UNITS',1,0,'x'*32,'bad row'),
    ]:
        t=fixture();t[section][row][field]=value;check(name,material(t),error=error)
    for reactor in ['-1','5','junk']:
        t=fixture();t['UNITS'][1]+=[reactor];check('bad-reactor-'+reactor,material(t),error='bad number')
    for tail in [['1','extra'],['','1']]:
        t=fixture();t['UNITS'][1]+=tail;check('surplus-reactor-'+str(len(results)),material(t),error='bad row' if tail[0]=='1' else 'bad number')
    check('nul',data.replace(b'#CHASSIS\n',b'#CHASSIS\n\x00\n'),error='bad row')
    check('long-line',data.replace(b'#CHASSIS\n',b'#CHASSIS\n'+b'x'*600+b'\n'),error='line too long')
    if args.owner_dir:
        for variant,name in [('base','alpha'),('expansion','alphax')]:check('owner-'+variant,(args.owner_dir/(name+'.txt')).read_bytes(),variant)
    receipt=dict(command=command,runner=runner,cases=results,executableSHA256=hashlib.file_digest(exe.open('rb'),'sha256').hexdigest(),scope='Component definitions and raw unit blueprints only, not derived prototype stats or gameplay')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(results)} unit-definition cases passed')


if __name__=='__main__':main()
