#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,os,random,struct,subprocess
from test_original_world import valid_fixture
from test_original_game import decode
from test_production_loans import model as settle
from test_technology_effects import model as effects
from test_units import sections
SRC=Path(__file__).resolve().parent
NAMES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','technology_effects.cpp','production_loans.cpp','world_production_prefix.cpp','world_production_prefix_probe.cpp']

def rules(mode):
 rng=random.Random(2026102615+mode);keys=['None','Disable','T0','T1']
 return ('#TECHNOLOGY\n'+'\n'.join(f'Tech {i},T{i},0,1,2,3,{rng.choice(keys)},{rng.choice(keys)},'+format(rng.randrange(512),'b') for i in range(89))+'\n#END\n').encode()

def fixture(version,mode,vehicles=3,bases=2):
 rng=random.Random(version*100+mode);b=bytearray(valid_fixture(version,vehicles,bases));_,o,_=decode(b);fs=0x20bc if version<11 else 0x20cc;pref=o['legacy'] if version<11 else o['preferences'];shift=0x7c if version<11 else 0;b[pref+0x59]=[0,255,rng.getrandbits(8)][mode%3]
 for t in range(89):b[pref+0x1e0-shift+t]=rng.getrandbits(8)
 for f in range(8):
  p=o['factions']+f*fs;d=o['faction_definitions']+f*0x59c
  struct.pack_into('<i',b,p+0x220,rng.randrange(-100,201));struct.pack_into('<i',b,p+0xc0,rng.choice([-1,0,0,0,1]));struct.pack_into('<i',b,p+0x24c,rng.randrange(-5,10));struct.pack_into('<i',b,d+0x504,rng.randrange(-30,31));struct.pack_into('<i',b,d+0x50c,mode%9)
  for j in range(8):
   struct.pack_into('<I',b,p+0x18+4*j,rng.getrandbits(32));struct.pack_into('<i',b,p+0xc4+4*j,rng.randrange(-100,201));struct.pack_into('<i',b,p+0xe4+4*j,rng.randrange(-50,51));struct.pack_into('<i',b,d+0x510+4*j,rng.choice([0,3,7,8,9,15]));struct.pack_into('<i',b,d+0x530+4*j,rng.randrange(-20,21))
 if mode==30:
  for f in range(8):struct.pack_into('<i',b,o['factions']+f*fs+0x220,2**31-1);struct.pack_into('<i',b,o['factions']+f*fs+0xc0,0)
  b[pref+0x59]=255
 if mode==31:struct.pack_into('<i',b,o['faction_definitions']+0x59c+0x50c,9)
 if mode==32:struct.pack_into('<i',b,o['faction_definitions']+2*0x59c+0x504,2**31-1)
 return bytes(b)

def proposal(text,data,phase):
 info,o,sp=decode(data);old=int(info['version'])<11;fs=0x20bc if old else 0x20cc;bad=lambda e:struct.pack('<I',e)+bytes([90])*(fs+36)
 if not 0<=phase<8:return bad(1)
 pref=sp['legacy'] if old else sp['preferences'];shift=0x7c if old else 0;factions=[sp['factions'][i*fs:(i+1)*fs] for i in range(8)];selected=bytearray(factions[phase]);d=sp['faction_definitions'][phase*0x59c:(phase+1)*0x59c];rows=[];lines=sections(text)['TECHNOLOGY'];keys={r[1].lower():i for i,r in enumerate(lines)};keys.update(none=-1,disable=-2)
 for i,r in enumerate(lines):rows += [int(r[8],2),keys[r[6].lower()],keys[r[7].lower()],pref[0x1e0-shift+i]]
 count=struct.unpack_from('<i',d,0x50c)[0];bonus=[x for j in range(8) for x in [struct.unpack_from('<i',d,0x510+4*j)[0],struct.unpack_from('<i',d,0x530+4*j)[0]]];h=[phase,struct.unpack_from('<i',d,0x504)[0],struct.unpack_from('<i',selected,0x24c)[0],count,*bonus,*rows];err,cache=effects(h)
 if err:return bad(err)
 credits=[struct.unpack_from('<i',f,0x220)[0] for f in factions];balance=list(struct.unpack_from('<8i',selected,0xc4));payment=list(struct.unpack_from('<8i',selected,0xe4));diplomacy=list(struct.unpack_from('<8I',selected,0x18));sanctions=[struct.unpack_from('<i',f,0xc0)[0] for f in factions];settled=settle([phase,pref[0x59],*credits,*balance,*payment,*diplomacy,*sanctions]);err=struct.unpack_from('<I',settled)[0]
 if err:return bad(err)
 words=list(struct.unpack_from('<17i',settled,4));credits=words[:8];balance=words[8:16]
 def word(at,v):struct.pack_into('<I',selected,at-4 if old and at>=0x464 else at,v&0xffffffff)
 word(0,struct.unpack_from('<I',selected)[0]&~32);word(0x220,credits[phase])
 for j,v in enumerate(balance):word(0xc4+4*j,v)
 for at in [0x2fc,0x300,0x308,0x3f8]:word(at,0)
 for at,value in zip([0x304,0x314,0x318,0x31c,0x320],cache):word(at,value)
 for at in range(0x690,0x808,4):word(at,0)
 return bytes(4)+selected+struct.pack('<9i',*credits,words[-1])

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--owner-dir',type=Path);ap.add_argument('--scenario-dir',type=Path);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);command=[a.cc,*flags,*[str(SRC/n) for n in NAMES],'-o',str(out/'probe')];subprocess.run(command,check=True);digest=hashlib.sha256();records=[];accepted=rejected=guards=0
 cases=[(f'v{v}-{mode}','base' if v<11 else 'expansion',rules(mode),fixture(v,mode)) for v in range(7,13) for mode in range(33)]+[(f'empty{v}','base' if v<11 else 'expansion',rules(0),fixture(v,0,0,0)) for v in [7,12]]
 if a.owner_dir:
  assert a.scenario_dir
  cases += [('owner-'+name,'base',(a.owner_dir/'alpha.txt').read_bytes(),(a.scenario_dir/name).read_bytes()) for name in ['Explore.SC','Build.SC','conquer.SC']]
 for name,profile,text,data in cases:
  info,_,_=decode(data);version=int(info['version']);tp=out/(name+'.txt');sp=out/(name+'.SC');qp=out/(name+'.queries');tp.write_bytes(text);sp.write_bytes(data);qp.write_bytes(struct.pack('<10i',*range(8),-1,8));fs=0x20bc if version<11 else 0x20cc;answers=[proposal(text,data,f) for f in [*range(8),-1,8]];expected=b''.join(answers);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),profile,str(tp),str(sp),str(qp)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(name,r.returncode,r.stderr);assert r.stdout[:-4]==expected,(name,len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));g=struct.unpack_from('<I',r.stdout,len(expected))[0];assert g>=14;guards+=g;digest.update(r.stdout);assert tp.read_bytes()==text and sp.read_bytes()==data;accepted+=sum(x[:4]==bytes(4) for x in answers);rejected+=sum(x[:4]!=bytes(4) for x in answers);records.append(dict(name=name,inputSHA256=hashlib.sha256(data).hexdigest(),outputSHA256=hashlib.sha256(r.stdout).hexdigest()))

 receipt=dict(worlds=len(records),queries=accepted+rejected,accepted=accepted,rejected=rejected,guards=guards,outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=command,records=records);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
