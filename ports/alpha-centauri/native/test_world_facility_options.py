#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,copy,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from test_original_game import decode
from test_original_world import valid_fixture
from test_facility_definitions import tables,material,sections
from test_facility_availability import expected
HERE=Path(__file__).resolve().parent
SOURCES=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','movement.cpp','saved_speed.cpp','world_unit_stats.cpp','facility_availability.cpp','world_facility_options.cpp','world_facility_options_probe.cpp']

def model(rules,data,profile,enabled):
 info,off,spans=decode(data);version=int(info['version']);baseprofile=profile=='base';fac=sections(rules);tech=fac['technology'];keys={r[1].lower():i for i,r in enumerate(tech)};keys.update(none=-1,disable=-2);prereq=[keys[r[3].lower()] for r in fac['facilities']];pref=off['preferences'] if version>=11 else off['legacy'];shift=0 if version>=11 else 0x7c;count=72 if baseprofile else 134;first=39 if baseprofile else 70;nbase=int(info['bases']);width=int(info['width']);height=int(info['height']);flat=struct.unpack_from('<I',data,off['metadata']+28)[0]!=0;bs=308 if version>=11 else 304;owners=data[pref+0x1e0-shift:pref+0x1e0-shift+89];projectloc=list(struct.unpack_from('<'+('64i' if version>=11 else '33i'),data,pref+0x84));out=bytearray()
 for faction in range(1,8):
  fdef=off['faction_definitions']+faction*0x59c;filename=data[fdef+4:fdef+28].split(b'\0')[0];flags=struct.unpack_from('<i',data,fdef+0x508)[0]
  for base in range(-1,nbase):
   for queuecount in (0,1,10):
    error=(version<11)!=baseprofile or (baseprofile and enabled) or (not baseprofile and any(c>=128 for c in filename))
    projects=[]
    for b in projectloc:
     if b< -2 or b>=nbase:error=True;projects.append(-1)
     else:projects.append(b if b<0 else data[off['bases']+b*bs+4])
    projects += [-1]*(64-len(projects));coast=False;bits=[0]*12;queue=[0]*10
    if base>=0:
     bp=off['bases']+base*bs;bits=list(data[bp+0x8c:bp+0x8c+(12 if version>=11 else 8)])+[0]*(0 if version>=11 else 4);queue=list(struct.unpack_from('<10i',data,bp+0x50));x,y=struct.unpack_from('<hh',data,bp)
     for dx,dy in ((1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)):
      tx=x+dx;ty=y+dy
      if not flat:tx%=width
      if 0<=tx<width and 0<=ty<height and data[off['tiles']+((ty*width+tx)//2)*44]&0xe0<0x60:coast=True
    known=[]
    for i,row in enumerate(tech):
     a,b=(keys[row[j].lower()] for j in (6,7));known.append(int(i<88 and a!=-2 and not(b==-2 and a!=-1) and bool(owners[i]&(1<<faction))))
    h=[int(not baseprofile),int(enabled),int(filename.upper()==b'CARETAKE'),int(base>=0),int(coast),faction,struct.unpack_from('<i',data,pref+0x34)[0],queuecount,struct.unpack_from('<i',data,pref+0xc)[0],flags,0,-1];values=[]
    for i in range(count):
     h[10]=i;h[11]=-1 if not i else prereq[i-1];status,value=expected({'head':h,'facilities':bits,'known':known,'projects':projects,'queue':queue});error|=bool(status);values.append(value)
    out+=struct.pack('<6i',faction,base,queuecount,int(bool(error)),0 if error else count,0 if error else int(coast))+bytes(134) if error else struct.pack('<6i',faction,base,queuecount,0,count,int(coast))+bytes(values)+bytes(134-count)
 return bytes(out)

def fixture(version,case):
 bases=0 if case==31 else 2;b=bytearray(valid_fixture(version,0,bases));info,off,spans=decode(b);pref=off['preferences'] if version>=11 else off['legacy'];shift=0 if version>=11 else 0x7c;bs=308 if version>=11 else 304;projects=64 if version>=11 else 33;width=int(info['width']);height=int(info['height']);rng=random.Random(717+version*100+case)
 struct.pack_into('<i',b,pref+0xc,[-2147483648,2048,0,0][case%4]);struct.pack_into('<i',b,pref+0x34,case%6 if case!=32 else 6);struct.pack_into('<I',b,off['metadata']+28,case%2)
 for i in range(projects):struct.pack_into('<i',b,pref+0x84+4*i,-1)
 if bases:
  for i in (1,5,6,10,17,27,28,31,32,35,36):
   if i<projects:struct.pack_into('<i',b,pref+0x84+4*i,[-1,-2,0,1][(case+i)%4])
  if case in (33,34):struct.pack_into('<i',b,pref+0x84,[-3,2][case-33])
 for i in range(89):b[pref+0x1e0-shift+i]=rng.randrange(256)
 for faction in range(8):
  fdef=off['faction_definitions']+faction*0x59c;b[fdef+4:fdef+28]=bytes(24);name=b'cArEtAkE' if faction==5 and case%3==0 else b'GAIANS'
  if faction==5 and case==35:name=b'G\xe9IANS'
  b[fdef+4:fdef+4+len(name)]=name;struct.pack_into('<I',b,fdef+0x508,[0,128,512,640][(case+faction)%4])
 for i in range(int(info['tiles'])) if 'tiles' in info else range(width//2*height):b[off['tiles']+44*i]=0x60
 for base in range(bases):
  bp=off['bases']+bs*base;x=0 if base==0 else width-2;y=0 if base==0 else height-2;struct.pack_into('<hh',b,bp,x,y);b[bp+4]=(case+base)%8;struct.pack_into('<i',b,bp+0x4c,(case%11)-1)
  for i in range(12 if version>=11 else 8):b[bp+0x8c+i]=rng.randrange(256) if case%4==3 else 0
  for i in range(10):struct.pack_into('<i',b,bp+0x50+i*4,-rng.randrange(72 if version<11 else 134))
  if case%4==0:b[off['tiles']+((y*width+x)//2)*44]=0x40 # Center alone is not coastal.
  elif case%4==1:b[off['tiles']+((y*width+(x-2)%width)//2)*44]=0x40 # Across wrap boundary.
  elif case%4==2:b[off['tiles']+(((y+1)%height*width+(x+1)%width)//2)*44]=0x40
 return bytes(b)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original-rules',type=Path);ap.add_argument('--original',type=Path,action='append',default=[]);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);binary=a.out/'world-facility-probe';flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror'];flags+=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'];subprocess.run([a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(binary)],check=True);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');records=[];digest=hashlib.sha256()
 def check(name,rules,data,profile,enabled):
  stem=str(len(records));rp=a.out/(stem+'.txt');sp=a.out/(stem+'.SC');rp.write_bytes(rules);sp.write_bytes(data);before=hashlib.sha256(data).hexdigest();wanted=model(rules,data,profile,enabled);r=subprocess.run((['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(binary),profile,str(int(enabled)),str(rp),str(sp)],capture_output=True,env=env,timeout=30);assert r.returncode==0,(name,r.returncode,r.stderr)
  if r.stdout!=wanted:
   for i in range(len(wanted)//158):
    if r.stdout[158*i:158*(i+1)]!=wanted[158*i:158*(i+1)]:raise AssertionError((name,i,r.stdout[158*i:158*i+24].hex(),wanted[158*i:158*i+24].hex(),r.stdout[158*i+24:158*(i+1)].hex(),wanted[158*i+24:158*(i+1)].hex()))
   raise AssertionError((name,'length',len(r.stdout),len(wanted)))
  assert hashlib.sha256(sp.read_bytes()).hexdigest()==before;digest.update(r.stdout);(a.out/(stem+'.bin')).write_bytes(r.stdout);records.append({'name':name,'profile':profile,'expansion':enabled,'queries':len(wanted)//158,'outputSHA256':hashlib.sha256(wanted).hexdigest(),'passed':True})
 for version in range(7,13):
  profile='base' if version<11 else 'expansion';tech,fac=tables(profile)
  for i in (1,3):tech[i][6:8]=['Disable','None'] if i==1 else ['T0','Disable']
  tech[2][6:8]=['None','Disable'];rules=material(tech,fac)
  for case in range(36):
   data=fixture(version,case);check('v'+str(version)+'-'+str(case),rules,data,profile,False)
   if version>=11:check('v'+str(version)+'-'+str(case)+'-enabled',rules,data,profile,True)
  other='expansion' if profile=='base' else 'base';ot,of=tables(other);check('v'+str(version)+'-wrong-runtime-profile',material(ot,of),fixture(version,0),other,False)
  if profile=='base':check('v'+str(version)+'-invalid-expansion-flag',rules,fixture(version,0),profile,True)
 if a.original:
  assert a.original_rules
  rules=a.original_rules.read_bytes()
  for p in a.original:
   data=p.read_bytes();check('original-'+p.name,rules,data,'base',False);assert p.read_bytes()==data
 receipt={'worlds':len(records),'queries':sum(x['queries'] for x in records),'invalidArgumentControls':6*len(records),'emptyWorldControls':len(records),'outputSHA256':digest.hexdigest(),'cases':records,'binarySHA256':hashlib.sha256(binary.read_bytes()).hexdigest()};(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k!='cases'}))
if __name__=='__main__':main()
