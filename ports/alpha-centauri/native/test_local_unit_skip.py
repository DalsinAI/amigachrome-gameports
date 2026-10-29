# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import test_resumed_commands as resumed
from test_units import sections,material,reference
from test_saved_speed import calculate
from test_original_faction import SCHEMA
HERE=Path(__file__).resolve().parent
SOURCES=resumed.SOURCES[:-1]+['local_unit_skip.cpp','local_unit_skip_probe.cpp']
def fixture(version,case):
 foreign=case>=27;case%=27
 variant='base' if version<11 else 'expansion';text=resumed.actions.rules() if version<11 else resumed.workers.rules('alien-crossfire');t=sections(text);road=[1,3,100][case%3];speed=[1,2,99][case//3%3];triad=case//9%3;t['RULES'][0][0]=str(road);t['CHASSIS'][0][8:10]=[str(speed),str(triad)];t['WEAPONS'][0][2:4]=['1','0'];text=material(t)
 b=bytearray(resumed.actions.fixture(version,1) if version<11 else resumed.workers.fixture(version,1));info,o,_=resumed.workers.e.population.decode(b);fs=0x20bc if version<11 else 0x20cc;schema=SCHEMA['LegacyFactionV10' if version<11 else 'Faction'];morale=next(f['offset'] for f in schema['fields'] if f['name']=='SE_morale');struct.pack_into('<i',b,o['factions']+fs+morale,0);struct.pack_into('<i',b,o['faction_definitions']+0x59c+0x4d8,0)
 for i in range(int(info['vehicles'])):b[o['vehicles']+i*52+14]=1
 id=int(info['vehicles'])-1;p=o['vehicles']+id*52;struct.pack_into('<h',b,p+10,64);struct.pack_into('<h',b,p+46,-1);b[p+36]=6 if case%2 else 3;b[p+16]=[0,1,255][case//2%3];b[p+40]=[0,100,255][case%3];b[p+17]=[0,1,4,24][case%4]
 u=o['prototypes']+64*52;b[u:u+52]=bytes(52);b[u+39]=1;b[u+42]=0
 if foreign:
  for i in range(int(info['vehicles'])):b[o['vehicles']+i*52+14]=2
 normalized=resumed.normalized(text,bytes(b));table=reference(text,variant);_,value=calculate([int(version>=11),64,0,0,1,0,0,road,b[p+16],0,b[p+36]],table);expected=bytearray(normalized)
 if value<=255 and not foreign:expected[p+40]=value
 return text,bytes(b),bytes(expected),2 if foreign else int(value>255)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/n) for n in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];h=hashlib.sha256();records=[]
 for version in range(7,13):
  for case in range(30):
   text,data,expected,status=fixture(version,case);i=len(records);rp=out/f'{i}.txt';sp=out/f'{i}.SC';rp.write_bytes(text);sp.write_bytes(data);r=subprocess.run([*runner,str(exe),str(rp),str(sp)],capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/f'{i}.bin').write_bytes(r.stdout);(out/f'{i}.stderr').write_bytes(r.stderr);(out/f'{i}.expected').write_bytes(expected);assert r.returncode==0 and r.stdout[8:]==expected and struct.unpack_from('<I',r.stdout,4)[0]==status,(version,case,status,r.returncode,r.stderr,next((j for j,(x,y) in enumerate(zip(r.stdout[8:],expected)) if x!=y),None));assert rp.read_bytes()==text and sp.read_bytes()==data;h.update(r.stdout);records.append(dict(version=version,case=case,status=status,guards=struct.unpack_from('<I',r.stdout)[0]))
 receipt=dict(worlds=len(records),accepted=sum(r['status']==0 for r in records),overflow=sum(r['status']==1 for r in records),notOwner=sum(r['status']==2 for r in records),guards=sum(r['guards'] for r in records),outputSHA256=h.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd,records=records,native='UNRUN');(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['command','records']}))
if __name__=='__main__':main()
