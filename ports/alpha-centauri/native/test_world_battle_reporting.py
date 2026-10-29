# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess
import test_world_battle_entry as parent
from test_original_game import decode
from test_units import sections,reference
SOURCES=[s for s in parent.SOURCES if s!='world_battle_entry_probe.cpp']+['attack_modifier_ledger.cpp','world_battle_reporting.cpp','world_battle_reporting_probe.cpp']
def controls():
 for name,variant,text,data in parent.controls():
  yield name,variant,text,data
  if name in ['base-0','expansion-0']:
   b=bytearray(data);_,o,_=decode(b)
   for f in range(8):
    at=o['faction_definitions']+0x59c*f;struct.pack_into('<i',b,at+0x50c,8)
    for i,v in enumerate([150,100,75,100,125,50,100,150]):struct.pack_into('<i',b,at+0x510+4*i,18);struct.pack_into('<i',b,at+0x530+4*i,v)
   yield name+'-factor-ledger',variant,text,bytes(b)
def expected(text,data):
 info,o,sp=decode(data);n=int(info['vehicles']);ed=int(info['version'])>=11;bs=308 if ed else 304;width=int(info['width']);tab=reference(text,'expansion'if ed else'base');ts=sections(text);pf=sp['preferences']if ed else sp['legacy'];raw,_=parent.expected(text,data);rows=[list(struct.unpack_from('<20I',raw,i*80))for i in range(n*n)];answer=[];accepted=lookups=saturated=zero_lookup=hasty_appends=0;maximum=0
 rule=lambda name:int(ts['RULES'][parent.parent.RULE_ROWS[name]][0])
 def project_owner(index):
  base=struct.unpack_from('<i',pf,0x84+4*index)[0];return sp['bases'][base*bs+4]if base>=0 else -1
 for a in range(n):
  av=sp['vehicles'][52*a:52*(a+1)];uid=struct.unpack_from('<h',av,10)[0];ap=sp['prototypes'][52*uid:52*(uid+1)];ax,ay=struct.unpack_from('<hh',av);ac=sp['tiles'][((ay*width+ax)//2)*44];fd=sp['faction_definitions'][av[14]*0x59c:(av[14]+1)*0x59c]
  for b in range(n):
   row=rows[a*n+b].copy()
   if row[0]:answer.append(row+[0]*59);continue
   bv=sp['vehicles'][52*b:52*(b+1)];duid=struct.unpack_from('<h',bv,10)[0];bp=sp['prototypes'][52*duid:52*(duid+1)];bx,by=struct.unpack_from('<hh',bv);tile=sp['tiles'][((by*width+bx)//2)*44:((by*width+bx)//2+1)*44];items=struct.unpack_from('<I',tile,8)[0];events=[]
   if items&32 and tile[0]>=64 and project_owner(16)==av[14]:events.append([338,50])
   if struct.unpack_from('<I',fd,0x508)[0]&0x400 and rule('combat_pct_fanatic_atk_bonus'):events.append([528,rule('combat_pct_fanatic_atk_bonus')])
   if ed:
    for i in range(struct.unpack_from('<i',fd,0x50c)[0]):
     if struct.unpack_from('<i',fd,0x510+4*i)[0]==18:events.append([1108,struct.unpack_from('<i',fd,0x530+4*i)[0]-100])
   # Accepted fixtures have no attacker abilities/grants and conventional ground
   # chassis. Prototype speed (not damaged/elite vehicle speed) sets Mobile/Open.
   asp=max(1,min(99,int(tab[f'chassis.{ap[36]}.speed'])));dsp=max(1,min(99,int(tab[f'chassis.{bp[36]}.speed'])))
   if rule('combat_pct_mobile_open_ground')and row[5]==2 and tile[5]&192<128 and asp>1 and dsp<=asp:events.append([611,rule('combat_pct_mobile_open_ground')])
   if rule('combat_pct_atk_road'):events.append([606,rule('combat_pct_atk_road')])
   if rule('combat_pct_atk_higher_elev')and ac&224>tile[0]&224:events.append([330,rule('combat_pct_atk_higher_elev')])
   slots=[[0x5a5a5a5a,0x5a5a5a5a]for _ in range(4)];count=0;trace=[]
   for label,percent in events:
    append=int(bool(percent)and count<4);trace.extend([label,percent,append]);saturated+=int(bool(percent)and not append);zero_lookup+=not percent
    if append:slots[count]=[label,percent];count+=1
   ledger=[count]+[x for slot in slots for x in slot]+[len(events)]+trace+[0]*(39-len(trace));penalty=row[16];append=int(row[18]!=0 and penalty!=0 and count<4);row[17]=count+append;row[19]=append
   if append:slots[count]=[339,penalty]
   final=[count+append]+[x for slot in slots for x in slot]+[len(events)+row[18]];answer.append(row+ledger+final);accepted+=1;lookups+=len(events)+row[18];hasty_appends+=append;maximum=max(maximum,len(events)+row[18])
 guards=15+(14 if int(info['bases'])else 13)if accepted else 15
 return b''.join(struct.pack('<79I',*(x&0xffffffff for x in row))for row in answer)+struct.pack('<I',guards),dict(queries=len(answer),accepted=accepted,lookupObligations=lookups,saturatedNonzeroLookups=saturated,zeroPercentLookups=zero_lookup,hastyAppends=hasty_appends,maxLookupEvents=maximum)
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-m68040','-static','-O2']if a.m68k_linux else['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all'];subprocess.run([a.cc,'-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(src/f)for f in SOURCES]+['-o',str(out/'probe')],check=True)
 digest=hashlib.sha256();totals={};worlds=0
 for name,variant,text,data in controls():
  d=out/name;d.mkdir();(d/'rules.txt').write_bytes(text);(d/'world.sc').write_bytes(data);want,counts=expected(text,data);(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(out/'probe'),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert r.returncode==0,(name,r.returncode,r.stderr[:1500]);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():totals[k]=max(totals.get(k,0),v)if k=='maxLookupEvents'else totals.get(k,0)+v
 assert totals['accepted']and totals['saturatedNonzeroLookups']and totals['zeroPercentLookups']and totals['hastyAppends'];j=dict(worlds=worlds,counts=totals,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),native='UNRUN');(out/'receipt.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j))
if __name__=='__main__':main()
