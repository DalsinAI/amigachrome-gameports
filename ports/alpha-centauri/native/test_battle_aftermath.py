# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import struct,json,hashlib,subprocess,os,argparse,inspect,ast
import test_land_resolution as parent
from test_original_game import decode
SRC=Path(__file__).resolve().parent
SOURCES=[s for s in parent.SOURCES if s!='land_resolution_probe.cpp']+['battle_bookkeeping.cpp','world_battle_bookkeeping.cpp','battle_aftermath.cpp','battle_aftermath_probe.cpp']
U=0xffffffff
# Independent preexisting body model with original caller present-zero pool setup.
env=dict(vars(parent));source=inspect.getsource(parent.expected).replace('pools=[int(profile==0),0x7fffffff,1,0x7fffffff,int(profile==0),0xfffffffb]','pools=[1,0,1,0,1,0]');exec(compile(source,'independent_body_model','exec'),env);body_model=env['expected']
tree=ast.parse((SRC/'test_battle_bookkeeping.py').read_text());mod=ast.Module(body=[f for f in tree.body if isinstance(f,ast.FunctionDef)and f.name in ['signed','expected']],type_ignores=[]);numenv={'U':U};exec(compile(mod,'independent_bookkeeping_model','exec'),numenv);book_model=numenv['expected']
def controls():
 for k,(name,variant,text,data)in enumerate(parent.controls()):
  if name in ['base-0','expansion-0']:yield name+'-held-source',variant,text,data
  b=bytearray(data);info,o,s=decode(b);width=int(info['width']);height=int(info['height']);n=int(info['vehicles']);modern=int(info['version'])>=11;fs=len(s['factions'])//8;bs=308 if modern else 304;bases={struct.unpack_from('<hh',s['bases'],i*bs)for i in range(int(info['bases']))};positions=[(x,y)for y in range(height)for x in range(y&1,width,2)if(x,y)not in bases];assert len(positions)>=n
  for j in range(n):
   struct.pack_into('<hh',b,o['vehicles']+j*52,*positions[j]);b[o['vehicles']+j*52+14]=1+j%7
  for f in range(2,8):
   b[o['factions']+f*fs:o['factions']+(f+1)*fs]=b[o['factions']+fs:o['factions']+2*fs]
   b[o['faction_definitions']+f*0x59c:o['faction_definitions']+(f+1)*0x59c]=b[o['faction_definitions']+0x59c:o['faction_definitions']+2*0x59c]
  for j in range(len(s['tiles'])//44):
   b[o['tiles']+j*44]=(b[o['tiles']+j*44]&31)|96;at=o['tiles']+j*44+8;struct.pack_into('<I',b,at,struct.unpack_from('<I',b,at)[0]&~1)
  for x,y in bases:
   at=o['tiles']+(y*(width//2)+x//2)*44+8;struct.pack_into('<I',b,at,struct.unpack_from('<I',b,at)[0]|1)
  for i in range(8):
   for j in range(8):
    struct.pack_into('<I',b,o['factions']+i*fs+0x18c+j*4,(0x7fffffff+i*71+j*13)&U);struct.pack_into('<I',b,o['factions']+i*fs+0x1ac+j*4,(0xffffffff+i*17+j*31)&U)
   rec=[0xa1a1a1a1,0xb2b2b2b2,0xc3c3c3c3,((k*37)>>i)&1,int((k+i)%3!=0),0xd4d4d4d4]
   for f,v in zip([0x150,0x158,0x15c,0x160,0x164,0x498],rec):struct.pack_into('<I',b,o['monuments']+i*0x4f4+f,v)
  yield name,variant,text,bytes(b)
def expected(text,data):
 info,o,s=decode(data);n=int(info['vehicles']);width=int(info['width']);modern=int(info['version'])>=11;fs=len(s['factions'])//8;bs=308 if modern else 304;nt=len(s['tiles'])//44;raw,_=body_model(text,data);pos=0;out=bytearray();counts=dict(accepted=0,rejected=0,held_scope=0,held_retreat_capture=0,held_monument_ui=0,attacker_deleted=0,defender_deleted=0,rounds=0,draws=0)
 coords=[struct.unpack_from('<hh',s['vehicles'],i*52)for i in range(n)];bases={struct.unpack_from('<hh',s['bases'],i*bs)for i in range(int(info['bases']))};flags=[struct.unpack_from('<I',s['faction_definitions'],i*0x59c+0x508)[0]for i in range(8)];state=[(0x7fffffff+i)&U for i in range(8)]+[U-i for i in range(8)]
 for f in [0x18c,0x1ac]:state +=[struct.unpack_from('<I',s['factions'],i*fs+f+j*4)[0]for i in range(8)for j in range(8)]
 for i in range(8):state +=[struct.unpack_from('<I',s['monuments'],i*0x4f4+f)[0]for f in [0x150,0x158,0x15c,0x160,0x164,0x498]]
 turn=struct.unpack_from('<I',s['preferences']if modern else s['legacy'],0x44)[0]
 for profile in range(2):
  for a in range(n):
   for b in range(n):
    begin=pos;row=list(struct.unpack_from('<21I',raw,pos));pos+=84;detail=None
    if row[0]==0:
     detail=struct.unpack_from('<9I',raw,pos);pos+=36+detail[7]*52+nt*44+8*(512+1024+12)+32+24
    held=any(c in bases or struct.unpack_from('<I',s['tiles'],(c[1]*(width//2)+c[0]//2)*44+8)[0]&1 or any(coords[j]==c for j in range(n)if j not in [a,b])for c in [coords[a],coords[b]])
    code=row[0]
    if held:code=1;counts['held_scope']+=1
    elif detail and detail[0]in [0,3]:code=1;counts['held_retreat_capture']+=1
    elif detail:
     af=s['vehicles'][a*52+14];bf=s['vehicles'][b*52+14];ctx=[int(modern),af,bf,0 if detail[0]==1 else 1,af if profile else U,0,0,1,turn,0x7fffffff]+flags;book=book_model(ctx+state);code=0 if book[0]==0 else 3 if book[0]==4 else 1
     if code==3:counts['held_monument_ui']+=1
    if code:
     out+=struct.pack('<21I',code,*([0]*20));counts['rejected']+=1;continue
    assert detail;out+=raw[begin:pos]+struct.pack('<387I',*book[1:]);after=book[193:385];mon=bytearray(s['monuments'])
    for i in range(8):
     for f,v in zip([0x150,0x158,0x15c,0x160,0x164,0x498],after[144+i*6:150+i*6]):struct.pack_into('<I',mon,i*0x4f4+f,v)
    out+=mon;counts['accepted']+=1;counts['attacker_deleted'if detail[0]==1 else'defender_deleted']+=1;counts['rounds']+=row[8];counts['draws']+=row[9]
 assert pos==len(raw)
 return bytes(out),counts

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--reuse',type=Path);args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent;flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if args.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);exe=out/'probe';subprocess.run([args.cc,*flags,*[str(src/f)for f in SOURCES],'-o',str(exe)],check=True);digest=hashlib.sha256();total={};worlds=0
 cs=((d.name,'expansion'if 'expansion'in d.name else'base',(d/'rules.txt').read_bytes(),(d/'world.sc').read_bytes())for d in sorted(args.reuse.iterdir(),key=lambda d:(d.name.split('-')[0],int(d.name.split('-')[1])if len(d.name.split('-'))>1 else 0,d.name))if d.is_dir())if args.reuse else controls()
 for name,variant,text,data in cs:
  d=out/name;d.mkdir();(d/'world.sc').write_bytes(data);(d/'rules.txt').write_bytes(text)
  if args.reuse:want=(args.reuse/name/'expected.bin').read_bytes();counts=json.loads((args.reuse/name/'counts.json').read_text())
  else:want,counts=expected(text,data)
  counts['runtime_no_write_guards']=8*int(bool(counts['accepted']));(d/'counts.json').write_text(json.dumps(counts));(d/'expected.bin').write_bytes(want);r=subprocess.run((['qemu-m68k','-cpu','m68040']if args.m68k_linux else[])+[str(exe),str(d/'rules.txt'),str(d/'world.sc')],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(d/'actual.bin').write_bytes(r.stdout);(d/'stderr').write_bytes(r.stderr);assert not r.returncode,(name,r.returncode,r.stderr);assert r.stderr==f"COMPOSITION_GUARD {int(bool(counts['accepted']))}\nRUNTIME_GUARDS {8*int(bool(counts['accepted']))}\n".encode(),(name,r.stderr);assert r.stdout==want,(name,len(r.stdout),len(want),next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));digest.update(r.stdout);worlds+=1
  for k,v in counts.items():total[k]=total.get(k,0)+v
 assert total['attacker_deleted'] and total['defender_deleted'] and total['held_monument_ui']
 receipt={'worlds':worlds,'counts':total,'outputSHA256':digest.hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))

if __name__=='__main__':main()
