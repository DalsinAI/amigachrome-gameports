# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned water-prefix/late-sprite composition; controls are not game art."""
from pathlib import Path
import argparse,subprocess,os,struct,json,hashlib
from test_terrain_scene import NAMES,fixture
from test_original_game import decode
from test_sprite_draw import reference
EXTRA=['sprite_scale','sprite_pixels','sprite_draw','terrain_sea_platform','terrain_sea_scene']
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-g'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['base','sea']:
  names=NAMES if name=='base'else NAMES[:-1]+EXTRA+['terrain_sea_scene_probe'];exe=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);bins[name]=hashlib.sha256(exe.read_bytes()).hexdigest()
 atlas=out/'atlas.bin';atlas.write_bytes(bytes(9 if x%7==0 or y%11==0 else (x*17+y*31)%236+10 for y in range(768)for x in range(1024)));(out/'light.bin').write_bytes(struct.pack('<80i',*[i*7%38 for i in range(80)]));(out/'metadata.bin').write_bytes(bytes(range(53))+bytes((i*5+k)%6 for i in range(53)for k in range(8)));(out/'resources.bin').write_bytes(bytes((i*17+i//56)%53 for i in range(3136))+bytes([21,37,53,69,85,101]));sprite=bytes(9 if(x+y)%5==0 else(x*19+y*37)%256 for y in range(40)for x in range(72));(out/'sprite.bin').write_bytes(sprite)
 b=fixture(2);_,o,_=decode(b)
 for i in range(24):
  at=o['tiles']+44*i;b[at]=((1+i%2)<<5)|((i%3)<<3);b[at+4]=2 if i%2 else 0;struct.pack_into('<I',b,at+8,(struct.unpack_from('<I',b,at+8)[0]&3)|0x8000|(0x20 if i%3 else 0));struct.pack_into('<I',b,at+12,0)
  for v in range(1,8):struct.pack_into('<I',b,at+12+4*v,0x8000 if i%3 else 0)
 f=out/'fixture.SC';f.write_bytes(b);queries=[(i,z,v,cx,cy,0x80,0x100,0x800000)for i in range(24)for z in[1,2]for v in[-1,1]for cx,cy in[(-12,20),(14,38),(-80,-30)]];packed=b''.join(struct.pack('<8i',*q)for q in queries);common=[str(f),'--synthetic',str(atlas),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];digest=hashlib.sha256();counts={};total=0
 for prefs in[0,0x100,0x200,0x300]:
  for mw,mh in[(0,0),(50,30)]:
   args=common+['0','0',str(prefs),'4','2'];base=subprocess.run(runner+[str(out/'base'),*args],input=packed,capture_output=True,env=env);assert base.returncode==0,(base.returncode,base.stderr);args[7]='1';sea=subprocess.run(runner+[str(out/'sea'),*args,str(out/'sprite.bin'),str(mw),str(mh)],input=packed,capture_output=True,env=env);assert sea.returncode==0,(sea.returncode,sea.stderr);assert len(base.stdout)==len(sea.stdout)==len(queries)*16388
   for j,q in enumerate(queries):
    i,z,v,cx,cy,*_=q;at=o['tiles']+i*44;initial=base.stdout[j*16388+4:(j+1)*16388];assert struct.unpack_from('<I',base.stdout,j*16388)[0]==0
    active=v<0 or(b[at+4]&(1<<v))and(struct.unpack_from('<I',b,at+12+4*v)[0]&0x8000)
    expected=reference((11 if z==1 else 21,16,72,40,14,4,100,62,cx,cy,0,0,128,128,mw,mh),sprite,initial)[36:]if active else initial
    status=struct.unpack_from('<I',sea.stdout,j*16388)[0];assert status==0,(prefs,mw,q,status);actual=sea.stdout[j*16388+4:(j+1)*16388];assert actual==expected,(prefs,mw,q,sum(x!=y for x,y in zip(actual,expected)));digest.update(sea.stdout[j*16388:(j+1)*16388]);counts[str(bool(active))]=counts.get(str(bool(active)),0)+1;total+=1
   if prefs==0x300 and mw==0:normal_for_padding=sea.stdout
 args=common+['0','1','768','4','2'];padded=subprocess.run(runner+[str(out/'sea'),*args,str(out/'sprite.bin'),'-6','0'],input=packed,capture_output=True,env=env);assert padded.returncode==0 and padded.stdout==normal_for_padding,(padded.returncode,padded.stderr)
 # Late failures must discard the already drawn private terrain prefix.
 one=struct.pack('<8i',1,1,-1,14,38,0x80,0,0);no_write=bytes([199])*16384;held=0
 for marker,expected in[(-2,1),(-3,1),(-4,8),(-5,8)]:
  r=subprocess.run(runner+[str(out/'sea'),*common,'0','1',str(out/'sprite.bin'),str(marker),'0'],input=one,capture_output=True,env=env);assert r.returncode==0,(marker,r.returncode,r.stderr);assert struct.unpack_from('<I',r.stdout)[0]==expected,(marker,r.stdout[:4]);assert r.stdout[4:]==no_write;held+=1
 r=subprocess.run(runner+[str(out/'sea'),*common,'0','33',str(out/'sprite.bin'),'0','0'],input=one,capture_output=True,env=env);assert r.returncode==0 and struct.unpack_from('<I',r.stdout)[0]==4 and r.stdout[4:]==no_write;held+=1
 land=out/'land.SC';land.write_bytes(fixture(1));land_common=[str(land),*common[1:]];land_digest=hashlib.sha256()
 for prefs in[0,0x100,0x200,0x300]:
  args=land_common+['0','1',str(prefs),'4','2'];base=subprocess.run(runner+[str(out/'base'),*args],input=packed,capture_output=True,env=env);sea=subprocess.run(runner+[str(out/'sea'),*args,str(out/'sprite.bin'),'0','0'],input=packed,capture_output=True,env=env);assert base.returncode==sea.returncode==0 and base.stdout==sea.stdout,(base.returncode,sea.returncode,base.stderr,sea.stderr);land_digest.update(sea.stdout)
 assert hashlib.sha256(padded.stdout).hexdigest()=='2db14157f3225367989a54d4803ba5accd455092e7b2909f10d164a5ec2adc00';assert digest.hexdigest()=='98fa32c27932ac0e8e9a235c104462f9f330cb54ae7a088bd29bbf2307455cce';assert land_digest.hexdigest()=='4ec1aef3e7e6d30e2158180b0a632ab418f5ed67ccfd103814efd9321e82ebe7'
 rec=dict(cases=total,paddedSurfaceCases=len(queries),paddedSurfaceSHA256=hashlib.sha256(padded.stdout).hexdigest(),landDelegationCases=len(queries)*4,landDelegationSHA256=land_digest.hexdigest(),sha256=digest.hexdigest(),activeContexts=counts,lateFailureAndHeldNoWrite=held,newAliasControls=12,inheritedParentAliasControls=True,bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
