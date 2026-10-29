#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh owned intake then original nutrient accounting; complete wire model."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_world_stable_intake as intake
from test_base_nutrients import expected as nutrient_expected
HERE=Path(__file__).resolve().parent
def rules(variant):
 t=intake.sections(intake.rules(variant));row,col,_=intake.ROW['nutrient_req_citizen'];t['RULES'][row][col]='2';return intake.material(t)
def fixture(variant,version,case):
 b=bytearray(intake.fixture(variant,version,case));_,off,_=intake.decode(b);legacy=version<11;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];fs=0x20bc if legacy else 0x20cc
 for name,value in [('SE_growth_pending',case%17-8),('nutrient_surplus_total',2147483647)]:
  f=next(f for f in schema['fields'] if f['name']==name);struct.pack_into('<i',b,off['factions']+fs+f['offset'],value)
 if case&1:b[off['bases']+0x8c]|=4
 if case&2:struct.pack_into('<I',b,off['bases']+0x30,4)
 struct.pack_into('<i',b,off['bases']+0x3c,[-2147483648,-1,0,1,100][case%5]);return bytes(b)
def model(variant,text,data,mode=0):
 fresh=intake.model(variant,text,data,mode);fail=lambda code:[code]+[0x5a5a5a5a]*84
 if fresh[0]:return fail(fresh[0])
 info,off,spans=intake.decode(data);legacy=int(info['version'])<11;fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];b=spans['bases'];faction=b[4];f=spans['factions'][fs*faction:fs*(faction+1)]
 row,col,limits=intake.ROW['nutrient_req_citizen'];req=max(limits[0],min(limits[1],int(intake.sections(text)['RULES'][row][col])))
 bits=int(bool(b[0x8c]&4))|int(bool(struct.unpack_from('<I',b,0x30)[0]&4))<<1
 fields=[struct.unpack_from('<b',b,6)[0],intake.scalar(f,schema,'SE_growth_pending'),fresh[7],7,3,req,struct.unpack_from('<i',b,0x3c)[0],intake.scalar(f,schema,'nutrient_surplus_total'),0,bits];n=nutrient_expected(fields)
 if n[0]:return fail(7 if n[0]==3 else 1)
 return fresh+n[1:]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',type=Path,action='append',default=[]);ap.add_argument('--original-rules',type=Path);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','vehicle_stack.cpp','tile_yields.cpp','world_tile_yields.cpp','world_worker_radius.cpp','world_terraform_ecology.cpp','world_stable_intake.cpp','base_nutrients.cpp','world_fresh_nutrients.cpp','world_fresh_nutrients_probe.cpp'];exe=a.out/'world-fresh-nutrients-probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in sources],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else []
 cases=[]
 for variant in ['base','expansion']:
  for case in range(1,65):cases.append((f'{variant}-{case}',variant,fixture(variant,7+case%4 if variant=='base' else 11+case%2,case)))
 for version in range(7,13):
  for variant in ['base','expansion']:cases.append((f'cross-v{version}-{variant}',variant,fixture(variant,version,31)))
 seed=fixture('expansion',12,64);_,off,_=intake.decode(seed)
 for name,offset,fmt,value in [('specialist',off['bases']+0x7c,'i',1),('mask0',off['bases']+0x78,'I',0),('maskhigh',off['bases']+0x78,'I',0x80000003),('governor',off['bases']+0x38,'I',0x80000040),('nonhuman',off['preferences']+0x58,'B',0),('no-centre',off['tiles']+52*44+8,'I',0),('fungus',off['tiles']+44*44+8,'I',0x20),('stack',off['tiles']+8,'I',2),('project',off['preferences']+0x84+27*4,'i',99)]:
  b=bytearray(seed);struct.pack_into('<'+fmt,b,offset,value);cases.append((name,'expansion',bytes(b)))
 for name,rate,physical,queue in [('growth-max-overflow',2147483647,True,0),('queued-creche-only',2147483647,False,-2),('growth-max-no-facility',2147483647,False,0),('growth-min-creche',-2147483648,True,0)]:
  b=bytearray(seed);f=next(f for f in intake.FACTIONS['Faction']['fields'] if f['name']=='SE_growth_pending');struct.pack_into('<i',b,off['factions']+0x20cc+f['offset'],rate);b[off['bases']+0x8c]=(b[off['bases']+0x8c]&~4)|(4 if physical else 0);struct.pack_into('<i',b,off['bases']+0x50,queue);cases.append((name,'expansion',bytes(b)))
 for variant in ['base','expansion']:cases.append(('empty-'+variant,variant,intake.valid_fixture(12,0,0)))
 for p in a.original:cases.append(('owner-'+p.name,'base',p.read_bytes()))
 digest=hashlib.sha256();records=[];accepted=rejected=0
 for name,variant,data in cases:
  text=a.original_rules.read_bytes() if name.startswith('owner-') and a.original_rules else rules(variant);p=a.out/(name+'.SC');p.write_bytes(data);rp=a.out/(variant+'.txt');rp.write_bytes(text);r=subprocess.run([*runner,str(exe),variant,str(rp),str(p),'0'],capture_output=True,timeout=60,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));assert r.returncode==0,(name,r.returncode,r.stderr);want=model(variant,text,data);encoded=struct.pack('<85I',*[x&0xffffffff for x in want]);assert r.stdout==encoded,(name,struct.unpack('<85I',r.stdout),want);assert p.read_bytes()==data;digest.update(encoded);accepted+=want[0]==0;rejected+=want[0]!=0;records.append({'name':name,'error':want[0],'inputSHA256':hashlib.sha256(data).hexdigest(),'outputSHA256':hashlib.sha256(encoded).hexdigest()})
 receipt={'worlds':len(records),'accepted':accepted,'rejected':rejected,'outputSHA256':digest.hexdigest(),'allWorldInputsAndWireUnchanged':True,'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'records':records,'command':cmd,'native':'UNRUN'};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['records','command']}))
if __name__=='__main__':main()
