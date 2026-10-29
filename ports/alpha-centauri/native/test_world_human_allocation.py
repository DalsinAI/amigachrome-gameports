#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned world allocation and ordered fresh yield replay against independent wire model."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from world_human_allocation_reference import model,fixture,rules
from test_original_game import decode
from test_units import sections,material
HERE=Path(__file__).resolve().parent

def text(variant,case=0):
 t=sections(rules(variant))
 from test_world_tile_yields import ROW
 r,c,_=ROW['nutrient_req_citizen'];t['RULES'][r][c]='2'
 r,c,_=ROW['min_base_size_specialists'];t['RULES'][r][c]=str(case%17)
 t['CITIZENS']=[[f'Citizen{i}',f'Citizens{i}','None' if (case+i)%3 else 'T0','Disable',str(i%3),str((case+i)%4),str(i%5)] for i in range(7)]+[['Drone','Drones'],['Worker','Workers'],['Talent','Talents']]
 if case%4==0:
  for row in t['RESOURCEINFO']:row[1:4]=['5','4','3']
 if case%5==0:
  from test_world_tile_yields import ROW
  for key in ['tech_three_nutrients_sqr','tech_three_minerals_sqr','tech_three_energy_sqr']:
   r,c,_=ROW[key];t['RULES'][r][c]='Disable'
 return material(t)

def world(variant,version,case):
 b=bytearray(fixture(variant,version,case+1));_,o,_=decode(b);p=o['bases'];rng=random.Random(0x414c4c4f+case);pop=[0,1,2,3,5,16,17,20,31,63,127][case%11];workers=rng.randrange(1,min(21,pop+1)+1);special=rng.randrange(pop+2-workers)
 b[p+6]=pop;struct.pack_into('<I',b,p+0x78,(1<<workers)-1);struct.pack_into('<ii',b,p+0x7c,special,0);struct.pack_into('<II',b,p+0x84,0x1234560,0x6543210);struct.pack_into('<i',b,p+0x50,[-38,-39,-40,-69,-70,-71,0,511][case%8])
 if case%3==0:
  for i in [44,53]:struct.pack_into('<I',b,o['tiles']+44*i+8,0)
 return bytes(b)

def supported_world(variant,version,rating,count,clean):
 from test_original_faction import SCHEMA as FACTIONS
 b=bytearray(world(variant,version,8));_,o,_=decode(b);at=o['vehicles'];b[at:at]=bytes(52*count)
 pref=o['legacy'] if version<11 else o['preferences'];struct.pack_into('<i',b,pref+0x38,count);_,o,_=decode(b)
 schema=FACTIONS['LegacyFactionV10' if version<11 else 'Faction'];field=next(f['offset'] for f in schema['fields'] if f['name']=='SE_support_pending');stride=0x20bc if version<11 else 0x20cc;struct.pack_into('<i',b,o['factions']+stride+field,rating)
 proto=o['prototypes'];b[proto:proto+52]=bytes(52);struct.pack_into('<I',b,proto+32,(1<<16) if clean else 0);b[proto+39]=1
 for i in range(count):
  p=o['vehicles']+52*i;struct.pack_into('<hh',b,p,8,6);b[p+14]=1;b[p+39]=2;struct.pack_into('<hhh',b,p+46,0,i+1 if i+1<count else -1,i-1)
 p=o['tiles']+44*52+8;struct.pack_into('<I',b,p,struct.unpack_from('<I',b,p)[0]|2)
 return bytes(b)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--rules-data',nargs=2,action='append',default=[]);ap.add_argument('--original',nargs=3,action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 files=re.findall(r'"\$src/([^" ]+\.cpp)"',(HERE/'build_world_human_allocation_probe.sh').read_text());exe=out/'world-allocation';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);command=[a.cc,*flags,*[str(HERE/n) for n in files],'-o',str(exe)];subprocess.run(command,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];corpus=[]
 for version in range(7,13):
  variant='base' if version<11 else 'expansion'
  for case in range(48):corpus.append((f'v{version}-c{case}',variant,text(variant,case),world(variant,version,case),case%4,'synthetic'))
 for variant,version in [('base',7),('expansion',12)]:
  for rating in range(-4,4):
   for count in [3,7]:
    for clean in [False,True]:corpus.append((f'support-{variant}-{rating}-{count}-{clean}',variant,text(variant,8),supported_world(variant,version,rating,count,clean),0,'synthetic'))
 for variant,version in [('base',7),('expansion',12)]:
  for gov in [0x80000000,0x80dfbe01,0x80030000,0x8f000000,0x8f000040]:
   b=bytearray(world(variant,version,8));_,o,_=decode(b);p=o['bases'];b[p+6]=3;struct.pack_into('<I',b,p+0x78,15);struct.pack_into('<ii',b,p+0x7c,0,0);struct.pack_into('<I',b,p+0x38,gov);corpus.append((f'gov-retained-{variant}-{gov}',variant,text(variant,8),bytes(b),0,'synthetic'))
  b=bytearray(world(variant,version,8));_,o,_=decode(b);struct.pack_into('<I',b,o['tiles']+44*44+8,0x20);corpus.append(('fresh-fungus-'+variant,variant,text(variant,8),bytes(b),0,'synthetic'))
 seed=world('expansion',12,8);_,o,_=decode(seed)
 for name,offset,fmt,value in [('gov',o['bases']+0x38,'I',0x80000000),('adjust',o['bases']+0x80,'i',1),('high-mask',o['bases']+0x78,'I',0x80000001),('negative-special',o['bases']+0x7c,'i',-1),('no-centre',o['tiles']+52*44+8,'I',0),('nonhuman',o['preferences']+0x58,'B',0),('bad-stack',o['tiles']+8,'I',2),('bad-project',o['preferences']+0x84+27*4,'i',99),('ocean',o['tiles']+44*44,'B',0x40)]:
  b=bytearray(seed);struct.pack_into('<'+fmt,b,offset,value);corpus.append((name,'expansion',text('expansion',8),bytes(b),0,'synthetic'))
 for variant,path in a.rules_data:
  t=Path(path).read_bytes()
  for case in range(32):corpus.append((f'owner-rules-{variant}-{case}',variant,t,world(variant,7 if variant=='base' else 12,case),case%4,'owner-rules'))
 for variant,rp,sp in a.original:corpus.append((Path(sp).stem,variant,Path(rp).read_bytes(),Path(sp).read_bytes(),0,'owner-world'))
 digest=hashlib.sha256();records=[];accepted=changed=visits=0
 for index,(name,variant,t,b,mode,kind) in enumerate(corpus):
  rp=out/f'{index}.txt';sp=out/f'{index}.SC';rp.write_bytes(t);sp.write_bytes(b);wanted=model(variant,t,b,mode);encoded=struct.pack('<'+'I'*len(wanted),*[n&0xffffffff for n in wanted]);p=subprocess.run([*runner,str(exe),variant,str(rp),str(sp),str(mode)],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{index}.bin').write_bytes(p.stdout)
  assert p.returncode==0 and p.stdout==encoded,(name,p.returncode,p.stderr,struct.unpack('<'+'I'*(len(p.stdout)//4),p.stdout),wanted)
  assert rp.read_bytes()==t and sp.read_bytes()==b;digest.update(encoded);accepted+=wanted[0]==0
  if wanted[0]==0:changed+=wanted[107]!=wanted[8] or wanted[108]!=wanted[7];visits+=wanted[195]
  records.append(dict(name=name,kind=kind,mode=mode,error=wanted[0],outputSHA256=hashlib.sha256(encoded).hexdigest()))
 assert accepted>0,'No accepted allocation cases executed'
 receipt=dict(worlds=len(corpus),accepted=accepted,rejected=len(corpus)-accepted,changed=changed,visits=visits,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=command,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
