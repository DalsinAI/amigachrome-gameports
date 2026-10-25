#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Faction definition controls. Optional owner data stays outside repository/CI."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from test_social import section

BONUS = ('TECH MORALE PSI FACILITY RESEARCH DRONE TALENT ENERGY INTEREST COMMERCE '
         'POPULATION HURRY UNIT TECHCOST SHARETECH TERRAFORM SOCIAL ROBUST IMMUNITY '
         'IMPUNITY PENALTY FUNGNUTRIENT FUNGMINERALS FUNGENERGY COMMFREQ MINDCONTROL '
         'FANATIC VOTES FREEPROTO AQUATIC ALIEN FREEFAC REVOLT NODRONE WORMPOLICE '
         'FREEABIL PROBECOST DEFENSE OFFENSE TECHSHARE TECHSTEAL').split()
SCALARS = {1:'rule_morale',2:'rule_psi',4:'rule_research',5:'rule_drone',6:'rule_talent',
           7:'rule_energy',8:'rule_interest',9:'rule_commerce',10:'rule_population',
           11:'rule_hurry',13:'rule_techcost',14:'rule_sharetech',39:'rule_sharetech'}
SLOTS = {3:2,12:1,21:7,22:8,23:9,27:11,31:12,32:13,33:14,35:15,36:16,37:17,38:18}
FLAGS = {8:0x4000,15:0x2000,24:0x1000,25:0x800,26:0x400,28:0x200,
         29:0x100,30:0x80,34:0x40,39:0x20,40:0x10}


def fold(s):
    return s.translate(str.maketrans('ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'))


def first_rows(data, search, fallback):
    lines = data.decode('cp1252').splitlines()
    start = None
    for key in [search, fallback]:
        for i, line in enumerate(lines):
            if fold(line.strip()) == '#'+fold(key.lstrip('#')):
                start = i+1
                break
        if start is not None:
            break
    assert start is not None
    rows = []
    for line in lines[start:]:
        line = line.split(';',1)[0].strip()
        if line.startswith('#'):
            break
        if not line:
            continue
        row = [v.strip() for v in line.split(',')]
        if not row[-1]:
            row.pop()
        rows.append(row)
        if len(rows) == 8:
            break
    assert len(rows) == 8
    return rows


def number(s):
    return int(s,0) if fold(s).startswith(('0x','0b')) else int(s[2:] if fold(s).startswith('0d') else s,10)


def reference(rules, data, search, fallback, localized=False):
    rows = first_rows(data,search,fallback)
    tech = {fold(r[1]):i for i,r in enumerate(section(rules,'TECHNOLOGY'))}
    tech.update(none=-1,disable=-2)
    social = section(rules,'SOCIO')
    effects = {fold(key):i for i,key in enumerate(social[0])}
    categories = social[2]
    models = [[r[0] for r in social[3+c*4:7+c*4]] for c in range(4)]
    bonus_names = [k for row in section(rules,'BONUSNAMES') for k in row]
    kinds = {fold(key):i for i,key in enumerate(bonus_names)}
    header = rows[0]
    assert len(header)==12
    result = dict(zip(['formal_name_faction','desc_name_faction','noun_faction'],header[:3]))
    result.update(noun_gender=str({'f':1,'n':2}.get(fold(header[3])[0],0)),
                  is_noun_plural=str(int(number(header[4])>=2)),name_leader=header[5],
                  is_leader_female=str(int(fold(header[6])[0]=='f')))
    for key,value in zip(['ai_fight','ai_power','ai_tech','ai_wealth','ai_growth'],header[7:]):
        result[key]=str(number(value))
    for key in set(SCALARS.values())|{'rule_tech_selected'}:
        result[key]='100' if key in ['rule_hurry','rule_techcost'] else '0'
    flag=0
    slots=[]
    pairs=rows[1]
    assert len(pairs)%2==0
    for key,param in zip(pairs[::2],pairs[1::2]):
        kind=kinds[fold(key)]
        if kind in FLAGS:
            flag|=FLAGS[kind]
        if kind==0:
            try:
                value=number(param)
            except ValueError:
                value=0
            if value:
                result['rule_tech_selected']=str(value)
            else:
                slots.append((0,tech[fold(param)],0))
        elif kind in SCALARS:
            value=number(param)
            if kind==11:
                value=min(1000,max(1,value))
            result[SCALARS[kind]]=str(value)
            if kind==1 and value==0:
                flag|=0x8000
        elif kind in SLOTS:
            slots.append((SLOTS[kind],number(param),0))
        elif kind in [16,17,18]:
            effect=param.lstrip('+-')
            signs=param[:len(param)-len(effect)]
            amount=signs.count('+')-signs.count('-')
            slots.append(({16:3,17:10,18:4}[kind],effects[fold(effect)],amount or 1))
        elif kind in [19,20]:
            found=[(c,m) for c,group in enumerate(models) for m,name in enumerate(group) if fold(name)==fold(param)]
            assert len(found)==1
            slots.append((5 if kind==19 else 6,*found[0]))
        else:
            assert kind in FLAGS
    assert len(slots)<=8
    result['rule_flags']=str(flag)
    result['bonus_count']=str(len(slots))
    for i,slot in enumerate(slots):
        for key,value in zip(['kind','value','modifier'],slot):
            result[f'bonus.{i}.{key}']=str(value)
    def matches(a,b):
        return fold(a[:4] if localized else a)==fold(b[:4] if localized else b)
    for i,row in enumerate(rows[2:4]):
        assert len(row)==3
        category,model,effect=row
        c,m=-1,0
        if fold(category) not in ['nil','none']:
            found=[j for j,name in enumerate(categories) if matches(category,name)]
            assert len(found)==1
            c=found[0]
            found=[j for j,name in enumerate(models[c]) if matches(model,name)]
            assert len(found)==1
            m=found[0]
        else:
            assert fold(model) in ['nil','none']
        e=-1 if fold(effect) in ['nil','none'] else effects[fold(effect)]
        for key,value in [('category',c),('model',m),('effect',e)]:
            result[f'ideology.{i}.{key}']=str(value)
    assert len(rows[4]) in [1,2] and len(rows[5])==3 and len(rows[6])==5 and len(rows[7])==1
    result['adj_name_faction']=rows[4][0]
    result['abbreviation']=rows[4][1] if len(rows[4])==2 else ''
    for key,value in zip(['assistant_name','scientist_name','assistant_city'],rows[5]):
        result[key]=value
    for key,value in zip(['title_leader','adj_leader','adj_insult_leader','adj_faction','adj_insult_faction'],rows[6]):
        result[key]=value
    result['insult_leader']=rows[7][0]
    return result


def synthetic_rules(variant):
    tech='#TECHNOLOGY\n'+'\n'.join(f'Test {i},T{i},0,1,2,3,None,None,1' for i in range(89))+'\n'
    rows=[[f'E{i}' for i in range(11)],[f'Label{i}' for i in range(11)],[f'Cat{i}' for i in range(4)]]
    rows += [[f'M{c}{m}','None'] for c in range(4) for m in range(4)]
    keys=BONUS[:29 if variant=='base' else 41]
    return (tech+'#SOCIO\n'+'\n'.join(','.join(r) for r in rows)+'\n#BONUSNAMES\n'+
            '\n'.join(','.join(keys[i:i+8])+',' for i in range(0,len(keys),8))+'\n#END\n').encode()


def material(bonuses='TECH,T88',header=None,ideology=None,tail=None):
    return ('#TEST\n'+(header or 'Test Faction,Test description,Testers,N,2,Leader,F,-1,1,2,3,4,')+'\n'+
            bonuses+'\n'+(ideology or 'Cat1,M12,E3\nCat0,M03,nil')+'\n'+
            (tail or 'Testing,Test\nAssistant,Scientist,City\nChief,strong,rude,fearless,terrible,\nOpponent')+
            '\nUnparsed narrative after the first eight rows\n#BASES\nFirst base\n').encode('cp1252')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--cc',default='c++')
    ap.add_argument('--m68k-linux',action='store_true')
    ap.add_argument('--owner-dir',type=Path)
    args=ap.parse_args()
    source=Path(__file__).resolve().parent
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=False)
    executable=out/'faction-probe'
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']
    runner=[]
    if args.m68k_linux:
        flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
    else:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=[args.cc,*flags,str(source/'rules_text.cpp'),str(source/'faction_probe.cpp'),'-o',str(executable)]
    subprocess.run(command,check=True)
    results=[]
    def check(name,data,variant='expansion',rules=None,search='TEST',fallback='TEST',localized=False,error=None):
        rulebytes=synthetic_rules(variant) if rules is None else rules
        rulefile=out/(name+'-rules.txt');rulefile.write_bytes(rulebytes)
        factionfile=out/(name+'-faction.txt');factionfile.write_bytes(data)
        proc=subprocess.run([*runner,str(executable),variant,str(rulefile),str(factionfile),search,fallback,
                             *(['localized'] if localized else [])],text=True,encoding="cp1252",capture_output=True,timeout=20,
                            env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if error:
            assert proc.returncode==20 and error in proc.stderr,(name,proc.returncode,proc.stderr)
        else:
            assert proc.returncode==0,(name,proc.returncode,proc.stderr)
            actual=dict(line.split('=',1) for line in proc.stdout.splitlines())
            expected=reference(rulebytes,data,search,fallback,localized)
            assert actual==expected,(name,[(k,v,expected.get(k)) for k,v in actual.items() if expected.get(k)!=v])
        results.append(dict(case=name,passed=True,expected_error=error))
    data=material()
    for variant in ['base','expansion']:
        check(variant,data,variant)
        check(variant+'-crlf',data.replace(b'\n',b'\r\n'),variant)
        check(variant+'-cr',data.replace(b'\n',b'\r'),variant)
    check('fallback',data,search='Missing')
    check('section-priority',data+data.replace(b'#TEST',b'#OTHER').replace(b'Test Faction',b'Other Faction'),search='OTHER')
    check('missing-section',data,search='Missing',fallback='MissingAgain',error='missing section')
    check('case-fold',data.replace(b'T88',b't88').replace(b'Cat1',b'cAt1').replace(b'E3',b'e3'))
    for i,key in enumerate(BONUS):
        param=('T88' if i==0 else '+-E2' if i in [16,17,18] else 'M12' if i in [19,20] else '0' if i==1 else '-3')
        check('kind-'+key,material(key+','+param))
    for label,bonus in [('eight',','.join(['SOCIAL,+E0']*8)),('defaults','TECH,2'),
                         ('named-none','TECH,None'),('named-disabled','TECH,Disable'),('negative-selection','TECH,-2'),
                         ('repeated-morale','MORALE,0,MORALE,2'),('hurry-low','HURRY,-2147483648'),
                         ('hurry-high','HURRY,2147483647'),('unsigned-social','SOCIAL,E0'),
                         ('zero-social','SOCIAL,+-E0'),('modifier-signs','SOCIAL,--+E0'),
                         ('flag-parameter','FANATIC,ignored'),('mixed','TECH,2,TECH,T1,SOCIAL,++E0,IMMUNITY,E1,IMPUNITY,M12')]:
        check(label,material(bonus))
    for name,header in [('plural-min','Test,Description,Noun,M,-2147483648,Leader,M,0,0,0,0,0'),
                        ('plural-max','Test,Description,Noun,F,2147483647,Leader,F,0,0,0,0,0'),
                        ('latin1','Tést,Déscription,Noun,N,1,Léader,F,0,0,0,0,0')]:
        check(name,material(header=header))
    localized=material(ideology='Cat1 localized,M12x extended,E3\nnil,nil,nil')
    check('localized-match',localized,localized=True,rules=synthetic_rules('expansion').replace(b'M12,',b'M12x,'))
    check('exact-rejects-localized',localized,error='bad key')
    check('nil-ideologies',material(ideology='nil,nil,nil\nNone,None,None'))
    check('no-abbreviation',data.replace(b'Testing,Test\n',b'Testing\n'))
    for name,changed,error in [
        ('ninth-bonus',material(','.join(['SOCIAL,+E0']*9)),'too many faction bonuses'),
        ('unknown-bonus',material('UNKNOWN,0'),'bad key'),
        ('unknown-tech',material('TECH,Missing'),'unknown prerequisite'),
        ('zero-tech',material('TECH,0'),'unknown prerequisite'),
        ('unknown-effect',material('SOCIAL,+Absent'),'bad key'),
        ('unknown-model',material('IMPUNITY,Missing'),'bad key'),
        ('missing-parameter',material('TECH'),'bad key'),
        ('empty-parameter',material('TECH,,MORALE,1'),'bad key'),
        ('hidden-bonus',material('TECH,T1,,MORALE,1'),'bad key'),
        ('overflow',material('ENERGY,2147483648'),'bad number'),
        ('numeric-junk',material('ENERGY,12garbage'),'bad number'),
        ('long-name',data.replace(b'Test Faction',b'x'*40),'bad row'),
        ('extra-header',data.replace(b',3,4,\n',b',3,4,,hidden\n'),'bad row'),
        ('extra-ideology',data.replace(b'Cat1,M12,E3',b'Cat1,M12,E3,extra'),'bad row'),
        ('unknown-ideology-effect',data.replace(b'Cat1,M12,E3',b'Cat1,M12,Unknown'),'bad key'),
        ('unknown-ideology-model',data.replace(b'Cat1,M12,E3',b'Cat1,Unknown,E3'),'bad key'),
        ('unknown-ideology-category',data.replace(b'Cat1,M12,E3',b'Unknown,M12,E3'),'bad key'),
        ('bad-nil-model',data.replace(b'Cat1,M12,E3',b'nil,M12,E3'),'bad key'),
        ('extra-string',data.replace(b'Opponent\n',b'Opponent,extra\n'),'bad row'),
        ('nul',data.replace(b'#TEST\n',b'#TEST\n\x00\n'),'bad row'),
        ('long-line',data.replace(b'#TEST\n',b'#TEST\n'+b'x'*600+b'\n'),'line too long'),
    ]:
        check(name,changed,error=error)
    lines=data.splitlines()
    for count in range(8):
        check('truncated-'+str(count),b'\n'.join(lines[:count+1])+b'\n#END\n',error='bad row')
    # Original gameplay semantics use bonus index; translated key labels keep it.
    check('renamed-bonus',material('SCIENCE,T88'),rules=synthetic_rules('expansion').replace(b'TECH,',b'SCIENCE,'))
    if args.owner_dir:
        for variant,name,sections in [('base','alpha',['FACTIONS']),('expansion','alphax',['FACTIONS','NEWFACTIONS'])]:
            rules=(args.owner_dir/(name+'.txt')).read_bytes()
            for group in sections:
                for filename,search in section(rules,group):
                    check('owner-'+variant+'-'+filename.lower(),(args.owner_dir/(filename.lower()+'.txt')).read_bytes(),
                          variant,rules,search,filename)
    receipt=dict(command=command,runner=runner,cases=results,
                 executable_sha256=hashlib.file_digest(executable.open('rb'),'sha256').hexdigest(),
                 scope='Eight-row faction definition parser; not faction gameplay, narrative/assets, saves or original-executable fidelity')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(f'{len(results)} faction cases passed')


if __name__=='__main__':
    main()
