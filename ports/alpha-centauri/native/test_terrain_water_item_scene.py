# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,subprocess,os,struct,json,hashlib
from test_terrain_scene import NAMES,fixture
from test_original_game import decode
from test_sprite_draw import reference
from test_terrain_water_sprites import TRIMS,expected as late_expected
from test_terrain_sea_late import expected as intent
EXTRA=['sprite_scale','sprite_pixels','sprite_draw','terrain_sea_platform','terrain_sea_scene','terrain_sea_late','terrain_water_sprites','terrain_water_item_scene','terrain_water_item_scene_probe']
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);s=Path(__file__).resolve().parent;flags=['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-g'];runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');bins={}
 for name in['base','scene']:
  names=NAMES if name=='base'else NAMES[:-1]+EXTRA;exe=out/name;subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(s/(n+'.cpp'))for n in names],'-o',str(exe)],check=True);bins[name]=hashlib.sha256(exe.read_bytes()).hexdigest()
 atlas=out/'atlas.bin';atlas.write_bytes(bytes(9 if x%7==0 or y%11==0 else(x*17+y*31)%236+10 for y in range(768)for x in range(1024)));(out/'light.bin').write_bytes(struct.pack('<80i',*[i*7%38 for i in range(80)]));(out/'metadata.bin').write_bytes(bytes(range(53))+bytes((i*5+k)%6 for i in range(53)for k in range(8)));(out/'resources.bin').write_bytes(bytes((i*17+i//56)%53 for i in range(3136))+bytes([21,37,53,69,85,101]));sea=bytes(9 if(x+y)%5==0 else(x*19+y*37)%256 for y in range(40)for x in range(72));images=[bytes(9 if(x+y+i)%7==0 else(x*19+y*37+i*11)%256 for y in range(h)for x in range(w))for i,(w,h,ox,oy)in enumerate(TRIMS)];assets=out/'sprites.bin';assets.write_bytes(sea+b''.join(images))
 b=fixture(2);_,o,_=decode(b);types=[0x10,0x40,0x400000,0x800000,0x1000000,0x1c00050,0x8000,0x80000000]
 for i in range(24):
  at=o['tiles']+44*i;b[at]=((1+i%2)<<5)|((i%3)<<3);b[at+4]=2 if i%2 else 0;struct.pack_into('<I',b,at+8,types[i%8]);struct.pack_into('<I',b,at+12,0x1000 if i%3==0 else 0)
  for v in range(1,8):struct.pack_into('<I',b,at+12+4*v,types[(i+1)%8]if i%3 else 0)
 f=out/'fixture.SC';f.write_bytes(b);queries=[(i,z,v,cx,cy,128,256,0)for i in range(24)for z in[1,2]for v in[-1,1]for cx,cy in[(-12,20),(14,38)]];packed=b''.join(struct.pack('<8i',*q)for q in queries);common=[str(f),'--synthetic',str(atlas),str(out/'light.bin'),str(out/'metadata.bin'),str(out/'resources.bin')];digest=hashlib.sha256();counts={};total=0
 for prefs in[0,512]:
  args=common+['0','0',str(prefs),'4','2'];base=subprocess.run(runner+[str(out/'base'),*args],input=packed,capture_output=True,env=env);assert base.returncode==0,(base.returncode,base.stderr)
  for draw in[1,32,33]:
   for exp in[0,1]:
    args[7]=str(draw);r=subprocess.run(runner+[str(out/'scene'),*args,str(assets),'0','0',str(exp),'1'],input=packed,capture_output=True,env=env);assert r.returncode==0,(r.returncode,r.stderr);assert len(r.stdout)==len(base.stdout)==len(queries)*16388
    for j,q in enumerate(queries):
     i,z,viewer,cx,cy,*_=q;at=o['tiles']+44*i;items=struct.unpack_from('<I',b,at+8 if viewer<0 else at+16)[0];lm=struct.unpack_from('<I',b,at+12)[0];late=viewer<0 or bool(b[at+4]&(1<<viewer));ctx=(viewer,1,b[at+5]&7,i%4*2+((i//4)&1),i//4,-5 if z==1 else 5,b[at]>>5,draw,items,lm,int(late),exp,1,1);plan=intent(ctx);n=struct.unpack_from('<I',plan,4)[0];events=[struct.unpack_from('<2I',plan,28+k*8)[0]for k in range(n)];status=4 if 2 in events or 3 in events else 0;expected=bytes([199])*16384
     if status==0:
      expected=base.stdout[j*16388+4:(j+1)*16388]
      if 1 in events:expected=reference((11 if z==1 else 21,16,72,40,14,4,100,62,cx,cy,0,0,128,128,0,0),sea,expected)[36:]
      later=list(ctx);later[7]&=~1;extra=late_expected(tuple(later)+(cx,cy,0,0,0,0,128,128,1,1,63,0),images,expected);assert struct.unpack_from('<I',extra)[0]==0;expected=extra[4:]
     actual_status=struct.unpack_from('<I',r.stdout,j*16388)[0];actual=r.stdout[j*16388+4:(j+1)*16388];assert actual_status==status and actual==expected,(prefs,draw,exp,q,actual_status,status,sum(x!=y for x,y in zip(actual,expected)));counts[str(status)]=counts.get(str(status),0)+1;digest.update(r.stdout[j*16388:(j+1)*16388]);total+=1
 normal_padded=r.stdout;args=common+['0','33','512','4','2'];pr=subprocess.run(runner+[str(out/'scene'),*args,str(assets),'-6','0','1','1'],input=packed,capture_output=True,env=env);assert pr.returncode==0 and pr.stdout==normal_padded,(pr.returncode,pr.stderr)
 # Source base_at would repair a stale raw base bit, even at sea: retain hold.
 stale=bytearray(b);struct.pack_into('<I',stale,o['tiles']+3*44+8,1);sf=out/'stale.SC';sf.write_bytes(stale);cmd=[str(sf),*common[1:],'0','32',str(assets),'0','0','1','1'];one=struct.pack('<8i',3,1,-1,14,38,128,0,0);rr=subprocess.run(runner+[str(out/'scene'),*cmd],input=one,capture_output=True,env=env);assert rr.returncode==0 and rr.stdout==struct.pack('<I',4)+bytes([199])*16384,(rr.returncode,rr.stdout[:4],rr.stderr)
 held=1;one=struct.pack('<8i',4,1,-1,14,38,128,0,0)
 for marker,status in[(-2,1),(-3,1),(-4,8),(-5,8)]:
  rr=subprocess.run(runner+[str(out/'scene'),*common,'0','32',str(assets),str(marker),'0','1','1'],input=one,capture_output=True,env=env);assert rr.returncode==0 and rr.stdout==struct.pack('<I',status)+bytes([199])*16384,(marker,rr.returncode,rr.stdout[:4],rr.stderr);held+=1
 assert total==2304 and digest.hexdigest()=='5c5209eda1711fd90cafc96d94ca9b1f9a1dd190cb23e64b412461186f22a228'
 assert hashlib.sha256(pr.stdout).hexdigest()=='d1153c5d14916ef7390cb69de2fccd55f2f94560556bc5c3d26ab91354e1ed42'
 rec=dict(cases=total,paddedSurfaceCases=len(queries),paddedSHA256=hashlib.sha256(pr.stdout).hexdigest(),sha256=digest.hexdigest(),statusCounts=counts,inheritedParentControls=True,newAliasControls=18,lateFailureAndHeldNoWrite=held,bigEndian=a.m68k_linux,native=False,binaries=bins);(out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n');print(rec)
if __name__=='__main__':main()
