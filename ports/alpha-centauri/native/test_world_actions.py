#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Restricted clear-land integration: explicit unsupported and no-write gates.

Synthetic owned original containers only. This is not original order_veh or a
full turn-engine differential. Separate storage/price/budget source controls
cover those primitives; these cases check their integration boundary.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_original_game import decode,FORMATS
from test_original_faction import SCHEMA
from test_world_unit_stats import world_fixture,rules_fixture
from test_units import sections,material
SRC=Path(__file__).resolve().parent

def fixture(version=7,vehicles=1,bases=0):
 b=bytearray(world_fixture(version,0,vehicles,bases));info,o,_=decode(b)
 pref=o['preferences'] if version>=11 else o['legacy']
 struct.pack_into('<i',b,o['player'],5)
 struct.pack_into('<i',b,pref+0xc,0)  # GameRules wire field.
 for field in SCHEMA['MFaction']['fields']:
  if field['name'] in ('rule_flags','faction_bonus_count','rule_morale'):
   for faction in range(8):struct.pack_into('<'+FORMATS[field['type']],b,o['faction_definitions']+faction*0x59c+field['offset'],0)
 # The ordinary cleared-land subset explicitly removes all encounter triggers.
 for index in range(int(info['width'])*int(info['height'])//2):
  p=o['tiles']+44*index;b[p:p+44]=bytes(44);b[p]=0x60;b[p+2]=0xf0;b[p+4]=0xff;b[p+7]=0xff
  struct.pack_into('<I',b,p+8,0x100000)
 for i in range(vehicles):
  p=o['vehicles']+52*i;b[p:p+52]=bytes(52)
  struct.pack_into('<hh',b,p,2+2*i,2);struct.pack_into('<h',b,p+10,320);b[p+14]=5;b[p+36]=2
  struct.pack_into('<hhh',b,p+46,-1,-1,-1)
  tile=o['tiles']+((2*int(info['width'])+2+2*i)//2)*44
  b[tile+2]=0xf5;struct.pack_into('<I',b,tile+8,0x100002)
 p=o['prototypes']+320*52;b[p:p+52]=bytes(52);b[p+36]=1;b[p+39]=1;b[p+42]=3
 return bytes(b)

def controls():
 baseline=fixture();_,o,_=decode(baseline);v=o['vehicles'];tile=o['tiles'];width=8
 target=tile+((3*width+3)//2)*44
 cases=[('clear-land',baseline,[0,5,3,3,3],'moved',3)]
 def mutate(name,off,fmt,value,status='unsupported',price=0,args=None):
  b=bytearray(baseline);struct.pack_into('<'+fmt,b,off,value)
  cases.append((name,bytes(b),args or [0,5,3,3,3],status,price))
 for name,args,status in [('wrong-player',[0,6,3,3,3],'not_owner'),('native-player',[0,0,3,3,3],'bad_state'),('invalid-unit',[99,5,3,3,3],'bad_state'),('bad-road',[0,5,0,3,3],'bad_state'),('non-adjacent',[0,5,3,6,4],'bad_state'),('wrong-parity',[0,5,3,2,3],'bad_state')]:
  cases.append((name,baseline,args,status,0))
 for bit in [1,0x200,0x4000,0x1000000,0x2000000,0x8000000]:mutate('wake-state-'+hex(bit),v+4,'I',bit)
 for off in [17,18]:mutate('orders-'+str(off),v+off,'B',1)
 mutate('enemy-unit',v+14,'B',6,'not_owner')
 mutate('exhausted',v+40,'B',255,'exhausted',3)
 mutate('source-ocean',tile+((2*width+2)//2)*44,'B',0x40)
 mutate('destination-ocean',target,'B',0x40)
 for bits in [0x100020,0x102000,0x10000000,0x20]:mutate('encounter-'+hex(bits),target+8,'I',bits)
 mutate('foreign-territory',target+7,'b',6)
 mutate('unseen-target',target+4,'B',0)
 mutate('unseen-source',tile+((2*width+2)//2)*44+4,'B',0)
 mutate('unseen-neighbor',tile+((4*width+4)//2)*44+4,'B',0)
 mutate('broken-tile-presence',tile+((2*width+2)//2)*44+8,'I',0x100000,'bad_state',3)
 mutate('bad-stack-cycle',v+48,'h',0,'bad_state',3)
 mutate('ability',o['prototypes']+320*52+32,'I',0x400)
 mutate('non-combat-plan',o['prototypes']+320*52+42,'b',9)
 mutate('psi-design',o['prototypes']+320*52+37,'B',12)
 fields={f['name']:f['offset'] for f in SCHEMA['MFaction']['fields']}
 for owner in (1,5):
  b=bytearray(baseline);struct.pack_into('<h',b,v+10,64)
  b[o['prototypes']+64*52:o['prototypes']+65*52]=b[o['prototypes']+320*52:o['prototypes']+321*52]
  definition=o['faction_definitions']+owner*0x59c
  struct.pack_into('<i',b,definition+fields['faction_bonus_count'],1)
  struct.pack_into('<i',b,definition+fields['faction_bonus_id'],15)
  struct.pack_into('<i',b,definition+fields['faction_bonus_val1'],10)
  cases.append(('free-ability-owner'+str(owner),bytes(b),[0,5,3,3,3],'unsupported',0))
 # The destination may contain a friendly stack, but any foreign actor in the
 # local neighborhood defers contact/combat/ZOC to the unimplemented path.
 b=bytearray(fixture(vehicles=2));_,two,_=decode(b);b[two['vehicles']+52+14]=6
 cases.append(('foreign-neighbor',bytes(b),[0,5,3,3,3],'unsupported',0))
 b=bytearray(baseline);source=tile+((2*width+2)//2)*44
 struct.pack_into('<I',b,source+8,0x100006);struct.pack_into('<I',b,target+8,0x100004)
 cases.append(('road',bytes(b),[0,5,3,3,3],'moved',1))
 struct.pack_into('<I',b,source+8,0x10000a);struct.pack_into('<I',b,target+8,0x100008)
 cases.append(('mag-tube',bytes(b),[0,5,3,3,3],'moved',0))
 mutate('forest',target+8,'I',0x300000,'moved',6)
 b=bytearray(baseline);struct.pack_into('<I',b,target+8,0);struct.pack_into('<I',b,o['metadata']+8,3)
 cases.append(('generated-pod',bytes(b),[0,5,3,3,3],'unsupported',0))
 struct.pack_into('<I',b,o['legacy']+0xc,0x2000);struct.pack_into('<I',b,target+8,0x4000000)
 cases.append(('landing-pod',bytes(b),[0,5,3,3,3],'unsupported',0))
 cases.append(('expansion-deferred',fixture(12),[0,5,3,3,3],'unsupported',0))
 cases.append(('empty-world',fixture(vehicles=0),[0,5,3,3,3],'bad_state',0))
 # Complete ordinary costs are allowed; fractional attempts are deferred.
 mutate('rough',target+5,'B',0x80,'moved',6)
 b=bytearray(baseline);b[target+5]=0x80;b[v+40]=4
 cases.append(('fractional-deferred',bytes(b),[0,5,3,3,3],'unsupported',6))
 field=next(f for f in SCHEMA['MFaction']['fields'] if f['name']=='rule_flags')
 mutate('alien',o['faction_definitions']+5*0x59c+field['offset'],'I',0x80)
 return cases

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--probe',type=Path);ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 files=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','world_movement_price.cpp','move_budget.cpp','vehicle_stack.cpp','world_actions.cpp','world_actions_probe.cpp']
 exe=a.probe.resolve() if a.probe else out/'world-actions-probe';cmd=[a.cc,*flags,*[str(SRC/f) for f in files],'-o',str(exe)]
 if not a.probe:subprocess.run(cmd,check=True)
 rules=out/'rules.txt';tables=sections(rules_fixture('base'));tables['CHASSIS'][1][9]='0';rules.write_bytes(material(tables))
 records=[]
 for i,(name,data,args,status,price) in enumerate(controls()):
  path=out/f'{i}.SC';path.write_bytes(data)
  r=subprocess.run([*runner,str(exe),str(rules),str(path),*map(str,args)],capture_output=True,text=True,timeout=30,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
  expected=f'status={status} price={price} unchanged={int(status!="moved")} seed=abcdef01 rc=0\n'
  assert r.returncode==0 and r.stdout==expected,(name,r.returncode,r.stdout,r.stderr,expected)
  records.append(dict(name=name,status=status,price=price,passed=True))
 (out/'receipt.json').write_text(json.dumps(dict(command=cmd,runner=runner,cases=records,binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print(f'{len(records)} restricted world-action boundary cases passed')
if __name__=='__main__':main()
