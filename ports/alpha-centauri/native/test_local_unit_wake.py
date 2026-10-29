# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_local_unit_skip as skip
import test_ready_search as ready
from test_units import sections,material
from test_original_game import decode,FORMATS
from test_original_faction import SCHEMA
HERE=Path(__file__).resolve().parent
SOURCES=ready.SOURCES[:-1]+['world_unit_wake.cpp','local_unit_wake.cpp','local_unit_wake_probe.cpp']
ORDERS=[0,1,3,4,13,14,15,23,24,127,128,255]
STATES=[0,0x200,0x204,0x4000000,0xb004200,0xffffffff]
CASES=144
def fixture(version,case):
 text,data,_,_=skip.fixture(version,0);b=bytearray(data);info,o,sp=decode(b);p=o['vehicles']+(int(info['vehicles'])-1)*52;u=o['prototypes']+64*52;t=sections(text)
 order=ORDERS[case%12];state=STATES[case//12%6];auto=[11,12,0][case//24%3];turns=[0,1,2,3,4,7,8,255][case//3%8];raw=(case//6)%2;project=case//12%3;grant=case//18%4;road=[1,3,100][case//48];speed=1 if case<120 else 99
 t['RULES'][0][0]=str(road);t['CHASSIS'][0][8]='%d'%speed;t['ABILITIES'][0][2]='None' if grant!=3 else 'Disable';text=material(t)
 b[p+17]=order;struct.pack_into('<I',b,p+4,state);b[p+37]=turns;b[p+38]=auto;b[p+40]=123;struct.pack_into('<I',b,u+32,raw)
 f=o['faction_definitions']+0x59c
 def field(name,value,index=0):
  s=next(x for x in SCHEMA['MFaction']['fields'] if x['name']==name);fmt=FORMATS[s['type']];struct.pack_into('<'+fmt,b,f+s['offset']+index*struct.calcsize(fmt),value)
 field('faction_bonus_count',1 if grant else 0);field('faction_bonus_id',15);field('faction_bonus_val1',0)
 prefs=o['preferences'] if version>=11 else o['legacy']
 for idx in [2,8]:struct.pack_into('<i',b,prefs+0x84+4*idx,0 if project==1 else 1 if project==2 else -1)
 # Make project0 owned, project1 foreign, independently of unit design owner.
 bs=308 if version>=11 else 304;b[o['bases']+4]=1;b[o['bases']+bs+4]=2
 if case>=138:
  for i in range(int(info['vehicles'])):b[o['vehicles']+i*52+14]=2
 expected=bytearray(skip.resumed.normalized(text,bytes(b)));status=2 if case>=138 else 0
 if status==0:
  nspent=123;nturns=turns
  if 4<=order<24 and not state&0x4000000:
   value=speed*road
   if value>255:status=1
   else:
    nspent=(value-road)%256
    ability=bool(raw) or(version>=11 and grant in [1,2]);rate=4 if ability else 2
    if project==1:rate=rate*2 if order in [14,15] else rate*3//2
    nturns=max(0,turns-rate) if turns else 0
  if status==0:
   if state&0x200 and auto==11 and not state&4:nspent=0
   expected[p+40]=nspent;expected[p+37]=nturns;expected[p+17]=0;struct.pack_into('<I',expected,p+4,state&0xf4ffbdff)
 return text,bytes(b),bytes(expected),status

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in range(CASES):
   text,data,expected,status=fixture(version,case);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout[8:]==expected and struct.unpack_from('<I',r.stdout,4)[0]==status,(version,case,status,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout[8:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,status=status,guards=struct.unpack_from('<I',r.stdout)[0]))
 receipt=dict(worlds=len(records),accepted=sum(r['status']==0 for r in records),overflow=sum(r['status']==1 for r in records),notOwner=sum(r['status']==2 for r in records),guards=sum(r['guards'] for r in records),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
