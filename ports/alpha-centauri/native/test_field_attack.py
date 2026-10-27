#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent whole-container casualty compaction and source field battle oracle."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from test_world_field_combat import fixture as field_fixture,rules_fixture,model as strength_model,put_field
from test_original_game import decode
from test_original_faction import SCHEMA
from test_combat_crt import draws
from test_combat_rounds import expected as duel_model
HERE=Path(__file__).resolve().parent

def fixture(case,permutation=0):
 data,m=field_fixture(case,2);data=bytearray(data);_,o,_=decode(data);p=o['vehicles']+104;data[p:p]=bytes(208);struct.pack_into('<I',data,12+0x38,6);_,o,_=decode(data)
 records=[bytearray(data[o['vehicles']+52*i:o['vehicles']+52*(i+1)])for i in range(2)]
 for label in range(2,6):
  v=bytearray(52);struct.pack_into('<hh',v,0,0 if label<4 else 2*(label-3),0);struct.pack_into('<h',v,10,320+label);v[14]=5;v[39]=255;v[43]=0x30+label;struct.pack_into('<hhh',v,46,-1,-1,-1);records.append(v)
  proto=o['prototypes']+52*(320+label);data[proto:proto+52]=bytes(52);data[proto+39]=1;data[proto+42]=3
 orders=[[0,1,2,3,4,5],[1,0,2,3,4,5],[2,3,0,4,1,5]];order=orders[permutation%3];index={old:new for new,old in enumerate(order)}
 struct.pack_into('<h',records[2],48,index[3]);struct.pack_into('<h',records[3],50,index[2])
 records[4][17]=1;struct.pack_into('<h',records[4],20,index[0]);records[5][17]=1;struct.pack_into('<h',records[5],20,5)
 for new,old in enumerate(order):data[o['vehicles']+52*new:o['vehicles']+52*(new+1)]=records[old]
 for x,faction in [(0,5),(2,5),(4,5)]:
  tile=o['tiles']+44*(x//2);struct.pack_into('<I',data,tile+8,2);data[tile+2]=faction
 schema=SCHEMA['LegacyFactionV10']
 for faction in range(8):
  f=o['factions']+faction*0x20bc;put_field(data,f,schema,'total_combat_units',0)
  for unit in range(320,326):put_field(data,f,schema,'units_active',0,unit)
 for i in range(6):
  v=data[o['vehicles']+52*i:o['vehicles']+52*(i+1)];f=o['factions']+v[14]*0x20bc;unit=struct.unpack_from('<h',v,10)[0]
  data[f+0x854+unit]+=1
  total=struct.unpack_from('<i',data,f+0x1054)[0];struct.pack_into('<i',data,f+0x1054,total+1)
  struct.pack_into('<H',data,f+0xc54+2*unit,7)
 for faction in range(8):struct.pack_into('<i',data,o['monuments']+faction*0x4f4+6*0x38+0x10,1)
 selected=[-1,index[0],index[1],5][case%4];struct.pack_into('<i',data,len(data)-248+4,selected)
 return bytes(data),m,index[0],index[1]

def expected(data,m,seed,attacker,defender,mode='normal'):
 if mode in['short','ledger-overflow','unsealed']:return (dict(error={'short':'bad_buffer','ledger-overflow':'overflow','unsealed':'bad_snapshot'}[mode],unchanged='1',rc='0'),b'')
 b=bytearray(data);_,o,_=decode(data)
 if mode=='live':
  v=o['vehicles']+52*5;b[v+40]=1;b[v+17]=24;struct.pack_into('<hh',b,v+20,2,0);p=o['bases'];struct.pack_into('<i',b,p+0x50,-3);struct.pack_into('<i',b,p+0x40,11)
 strength=strength_model(m);off=int(strength['offense']);deff=int(strength['defense']);am=int(strength['am']);dm=int(strength['dm'])
 _,values=draws(seed,160);r=duel_model([off,deff,m['ar'],m['dr'],m['ad'],m['dd'],160,0],values);assert r[0]==0
 next_seed=draws(seed,r[9])[0];aw=bool(r[10]);loser=defender if aw else attacker;winner=attacker if aw else defender
 af=5;df=6;wf=af if aw else df;lf=df if aw else af
 p=o['vehicles']+52*attacker;q=o['vehicles']+52*defender
 struct.pack_into('<I',b,p+4,struct.unpack_from('<I',b,p+4)[0]|0x44);struct.pack_into('<I',b,q+4,struct.unpack_from('<I',b,q+4)[0]|4)
 b[p+16]=r[2];b[q+16]=r[3];b[p+40]+=3;b[p+39]|=1<<df
 ahp=10*m['ar']-m['ad'];dhp=10*m['dr']-m['dd']
 po=off*(r[4]if aw else ahp)*m['ar']//(ahp if aw else 10*m['ar']);pd=deff*(dhp if aw else r[5])*m['dr']//(10*m['dr']if aw else dhp)
 pm=am if aw else dm;promdraws=0
 def roll(limit):
  nonlocal next_seed,promdraws
  if limit<=1:return 0
  next_seed,val=draws(next_seed,1);promdraws+=1;return val[0]%limit
 check=pm<=1 or roll(po+pd)<=(pd if aw else po);raw=b[o['vehicles']+52*winner+36]
 promoted=check and raw<6 and pm<6 and(pm<=2 or roll(2)==0)
 if promoted:b[o['vehicles']+52*winner+36]+=1
 loser_record=data[o['vehicles']+52*loser:o['vehicles']+52*(loser+1)];lunit=struct.unpack_from('<h',loser_record,10)[0]
 tx,ty=struct.unpack_from('<hh',loser_record);tile=o['tiles']+44*((8*ty+tx)//2);struct.pack_into('<I',b,tile+8,struct.unpack_from('<I',b,tile+8)[0]&~2)
 fb=o['factions']+lf*0x20bc;wb=o['factions']+wf*0x20bc;b[fb+0x854+lunit]-=1;struct.pack_into('<H',b,fb+0xc54+2*lunit,8);struct.pack_into('<i',b,fb+0x1054,struct.unpack_from('<i',b,fb+0x1054)[0]-1)
 for offp in[0x18c,0x1ac]:struct.pack_into('<i',b,wb+offp+4*lf,struct.unpack_from('<i',b,wb+offp+4*lf)[0]+1)
 excluded=0x4000000|0x1000000|0x100000|0x10000|0x800|0x400|0x80|7|0x800000
 for faction,other in[(af,df),(df,af)]:
  pos=o['factions']+faction*0x20bc+0x18+4*other;struct.pack_into('<I',b,pos,(struct.unpack_from('<I',b,pos)[0]&~excluded)|0x80008118)
 for actor,other in[(attacker,df),(defender,af)]:
  unit=struct.unpack_from('<h',data,o['vehicles']+52*actor+10)[0];b[o['prototypes']+52*unit+0x2d]|=1<<other
 for actor in range(6):
  if actor==loser:continue
  pos=o['vehicles']+52*actor
  for ref in[48,50]:
   n=struct.unpack_from('<h',b,pos+ref)[0]
   if n>loser:struct.pack_into('<h',b,pos+ref,n-1)
  if b[pos+17]==1:
   target=struct.unpack_from('<h',b,pos+20)[0]
   if target==loser:b[pos+17]=0
   elif target>loser:struct.pack_into('<h',b,pos+20,target-1)
 ui=len(b)-248+4;selected=struct.unpack_from('<i',b,ui)[0];selected=-1 if selected==loser else selected-1 if selected>loser else selected;struct.pack_into('<i',b,ui,selected)
 struct.pack_into('<i',b,12+0x38,5);del b[o['vehicles']+52*loser:o['vehicles']+52*(loser+1)]
 info,_,_=decode(b);assert info['vehicles']=='5'
 summary=dict(error='ok',bytes=str(len(b)),seed=str(next_seed),loser=str(loser),winner=str(winner-1 if winner>loser else winner),wf=str(wf),lf=str(lf),selected=str(selected),promoted=str(int(promoted)),promotion_draws=str(promdraws),rounds=str(r[6]),draws=str(r[9]),ad=str(r[2]),dd=str(r[3]),wins=str(101+wf),losses=str(21+lf),movement_seed=str(0xabcdef01),unchanged='1',rc='0')
 return summary,bytes(b)

def controls():
 rows=[]
 for case in range(12):
  for perm in range(3):
   data,m,aid,did=fixture(case,perm);seed=[0,1,0x80000000,0xffffffff][case%4];mode='live'if case%4==3 and perm!=2 else'normal';rows.append((f'field-{case}-{perm}',data,rules_fixture(case),seed,aid,did,mode,*expected(data,m,seed,aid,did,mode)))
 data,m,aid,did=fixture(2);rules=rules_fixture(2)
 for mode in['short','ledger-overflow','unsealed']:rows.append((mode,data,rules,1,aid,did,mode,*expected(data,m,1,aid,did,mode)))
 _,o,_=decode(data)
 def bad(name,pos,fmt,value,error):
  b=bytearray(data);struct.pack_into('<'+fmt,b,pos,value);rows.append((name,bytes(b),rules,1,aid,did,'normal',dict(error=error,unchanged='1',rc='0'),b''))
 summary,_=expected(data,m,1,aid,did);winner_faction=int(summary['wf']);loser_faction=int(summary['lf']);loser_id=int(summary['loser']);loser_unit=struct.unpack_from('<h',data,o['vehicles']+52*loser_id+10)[0]
 bad('first-kill-monument',o['monuments']+winner_faction*0x4f4+6*0x38+0x10,'i',0,'unsupported')
 bad('third-party-territory',o['tiles']+44*((8*2+2)//2)+7,'b',7,'unsupported')
 bad('objective-attacker',o['vehicles']+52*aid+8,'H',0x20,'unsupported')
 bad('invalid-ui-vehicle-ref',len(data)-248+4,'i',6,'bad_reference')
 bad('invalid-board-reference',o['vehicles']+52*4+20,'h',6,'bad_reference')
 bad('lost-counter-overflow',o['factions']+loser_faction*0x20bc+0xc54+2*loser_unit,'H',65535,'overflow')
 bad('empty-active-counter',o['factions']+loser_faction*0x20bc+0x854+loser_unit,'B',0,'bad_state')
 bad('empty-combat-counter',o['factions']+loser_faction*0x20bc+0x1054,'i',0,'bad_state')
 bad('inconsistent-active-counter',o['factions']+loser_faction*0x20bc+0x854+loser_unit,'B',5,'bad_state')
 bad('negative-winning-counter',o['factions']+winner_faction*0x20bc+0x18c+4*loser_faction,'i',-1,'bad_state')
 for slot in[0x18c,0x1ac]:bad(f'winning-counter-overflow-{slot}',o['factions']+winner_faction*0x20bc+slot+4*loser_faction,'i',2147483647,'overflow')
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='c++');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
 sources='rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack facility_production_snapshot combat_rounds combat_crt world_combat_sensors world_field_combat world_combat_crt field_attack_snapshot field_attack_probe'.split();exe=a.out/'field-attack-probe';cmd=[a.cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*[str(HERE/(s+'.cpp'))for s in sources],'-o',str(exe)];subprocess.run(cmd,check=True)
 records=[];aggregate=b''
 for i,(name,data,rules,seed,aid,did,mode,want,after)in enumerate(controls()):
  wp=a.out/f'{i}.SC';rp=a.out/f'{i}.txt';ap=a.out/f'{i}.after';wp.write_bytes(data);rp.write_bytes(rules);(a.out/f'{i}.expected').write_bytes(after)
  argv=[str(exe),str(rp),str(wp),hex(seed),str(aid),str(did),mode,str(ap)]
  run=subprocess.run(argv,capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True,timeout=30)
  got=dict(x.split('=')for x in run.stdout.split());assert got==want,(i,name,got,want)
  assert ap.read_bytes()==after,(i,name,'whole-container mismatch')
  (a.out/f'{i}.summary').write_text(run.stdout);aggregate+=after;records.append(dict(case=i,name=name,seed=seed,attacker=aid,defender=did,mode=mode,result=got,bytes=len(after),sha256=hashlib.sha256(after).hexdigest()))
 report=dict(cases=len(records),accepted=sum(r['result']['error']=='ok'for r in records),outputSHA256=hashlib.sha256(aggregate).hexdigest(),records=records,native='UNRUN',originalFullCombat='UNRUN');(a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='records'}))
if __name__=='__main__':main()
