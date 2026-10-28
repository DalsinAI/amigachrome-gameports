# SPDX-License-Identifier: GPL-3.0-or-later
"""Generic CVR vehicle assembly controls. All models, palettes and faction art
are generated synthetic fixtures; no original data is read or embedded."""
import argparse,json,os,re,subprocess
from pathlib import Path
from test_cvr_infantry import model
from test_cvr_lod import chain
from test_cvr_palette import table as faction_table

RED,GREEN,BLUE,WHITE=(255,0,0),(0,255,0),(0,0,255),(255,255,255)
GLOBAL=bytes(v for i in range(256) for v in ((i*17)%256,(i*37)%256,(i*67)%256))
NEUTRAL=bytes(range(256))*24  # synthetic models use an identity shade table
def g(i):return tuple(GLOBAL[i*3:i*3+3])
def remap(tint):
 # Colour index 5 through the reference faction table (shade-independent here).
 return faction_table(GLOBAL,NEUTRAL,tint)[5]

def views(text):
 out=[]
 for line in text.splitlines():
  if not line.startswith('VIEW '):continue
  m=re.match(r'VIEW zoom=(\d) facing=(\d) changed=(\d+) digest=(\w+)(.*)',line)
  colours={}
  for c in m.group(5).split():
   rgb,n=c.split('=');colours[tuple(map(int,rgb.split(',')))]=int(n)
  out.append(dict(zoom=int(m.group(1)),facing=int(m.group(2)),changed=int(m.group(3)),digest=m.group(4),colours=colours))
 return out

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args()
 out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 exe=out/'control'
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp')) for n in ['cvr_model','cvr_raster','cvr_lighting','cvr_palette','cvr_infantry','cvr_vehicle','cvr_vehicle_control']],'-lm','-o',str(exe)],check=True)
 def run(*args):
  cmd=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(exe),*map(str,args)]
  p=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True)
  assert p.returncode==0,(args,p.returncode,p.stdout,p.stderr);return p.stdout
 def write_parts(name,parts):
  d=out/name;d.mkdir()
  for i,data in enumerate(parts):(d/f'part{i}.cvr').write_bytes(data)
  return d
 report={}
 gpath=out/'global.pal';gpath.write_bytes(GLOBAL)

 # 1. Model-colour assembly is byte-identical to the qualified two/ten part profiles.
 d=write_parts('equiv-infantry',[model(RED,frames=15),model(BLUE,offset=(100,0,0))])
 assert 'views=32 identical' in run('equiv',d,'infantry');report['equivInfantryViews']=32
 d=write_parts('equiv-speeder',[model(RED if i%2 else BLUE,frames=1,offset=(i*40-200,0,0)) for i in range(10)])
 assert 'views=32 identical' in run('equiv',d,'speeder');report['equivSpeederViews']=32

 # 2. Four-component chassis/weapon/armour/reactor assembly in each colour mode.
 parts=[model(RED,frames=1),model(GREEN,frames=1,offset=(120,0,0)),model(BLUE,frames=1,offset=(0,120,0)),model(WHITE,frames=1,offset=(0,0,120))]
 d=write_parts('four',parts)
 common=[d,4]
 base=views(run('assemble',d,4,0,1.0,0,1,1000,'-','-',0))
 assert len(base)==16
 for v in base:
  assert set(v['colours'])<= {RED,GREEN,BLUE,WHITE} and RED in v['colours'],v
  assert sum(v['colours'].values())==v['changed']
 tint_a,tint_b=(128,173,227),(247,211,19)
 tints=bytearray(25);tints[6:9]=bytes(tint_a);tints[15:18]=bytes(tint_b);tints[24]=(1<<2)|(1<<5)
 tpath=out/'tints.bin';tpath.write_bytes(bytes(tints))
 cases=0
 for mask in [0,0b0010,0b1010,0b1111]:
  for faction in [0,2,5]:
   text=run('assemble',d,4,mask,1.0,0,1,1000,gpath,tpath,faction)
   got=views(text);assert len(got)==16 and 'INVALID rejected=8' in text
   tint={2:tint_a,5:tint_b}.get(faction)
   for v,b in zip(got,base):
    assert v['changed']==b['changed'],(mask,faction,v,b)  # geometry independent of colour
    want={}
    for part,colour in enumerate([RED,GREEN,BLUE,WHITE]):
     n=b['colours'].get(colour,0)
     if not n:continue
     index=remap(tint) if (tint and mask>>part&1) else 5
     want[g(index)]=want.get(g(index),0)+n
    assert v['colours']==want,(mask,faction,v['colours'],want)
   cases+=1
 report['colourModeCases']=cases;report['colourViews']=cases*16

 # 3. Rejected assemblies leave output unchanged.
 rejects=[
  ('body-frames',[model(RED,frames=1)],dict(frames=15)),
  ('voxel-limit',[model(RED,frames=1),model(BLUE,frames=1)],dict(limit=1)),
  ('zero-scale',[model(RED,frames=1)],dict(scale=0)),
  ('lod-change',[model(RED,frames=1),model(BLUE,frames=1,geom=chain((10,20)))],dict()),
  ('missing-part',[model(RED,frames=1)],dict(count=2)),
  ('thirteen-parts',[model(RED,frames=1)]*13,dict()),
 ]
 for name,ps,kw in rejects:
  d=write_parts('reject-'+name,ps)
  text=run('assemble',d,kw.get('count',len(ps)),0,kw.get('scale',1.0),0,kw.get('frames',1),kw.get('limit',1000),'-','-',0)
  assert 'LOAD rejected' in text,(name,text)
 for name,ps in [('all-hidden',[model(RED,frames=1,hidden=True)]*2),('singular',[model(RED,frames=1),model(BLUE,frames=1,singular=True)])]:
  d=write_parts('reject-'+name,ps)
  assert 'DRAW rejected' in run('assemble',d,2,0,1.0,0,1,1000,'-','-',0),name
 report['rejectedAssemblies']=len(rejects)+2

 # 4. Owner recipe table parser.
 good='# owner recipes\r\n2 1 0 1 1.5 -10 1 5000 : vb vgmcp* vr00\n\n  3 0 0 2 1 0 0 900 : a_1 b2* # trailing\n'
 recipe_cases=[('good',good,'RECIPES count=2 [2/1/0/1 1.5 -10 1 5000: vb vgmcp* vr00] [3/0/0/2 1 0 0 900: a_1 b2*]'),
  ('empty','# nothing\n','RECIPES count=0')]
 bad=['1 0 0 1 1 0 1 10 vb\n','1 0 0 1 1 0 1 10 : VB\n','1 0 0 1 1 0 1 10 : '+'a'*21+'\n','1 0 0 1 1 0 1 10 : '+' '.join(['p']*13)+'\n',
  '1 0 0 1 1 0 1 10 : a\n1 0 0 1 2 0 1 10 : b\n','-1 0 0 1 1 0 1 10 : a\n','1 0 0 1 0 0 1 10 : a\n','1 0 0 1 1 0 1 0 : a\n',
  '1 0 0 1 1 0 300 10 : a\n','1 0 0 1 nan 0 1 10 : a\n','1 0 0 1 1 0 1 10 : a\x01\n','1 0 0 1 1 0 1 10 :\n','1 0 0 1 1 0 1 10 : a-b\n',
  ''.join(f'{i} 0 0 1 1 0 1 10 : a\n' for i in range(65)),'1 0 0 1 1 0 1 10 : '+'a '*200+'\n']
 for i,text in enumerate(bad):recipe_cases.append((f'bad-{i}',text,'RECIPES rejected unchanged=1'))
 for name,text,expect in recipe_cases:
  path=out/f'recipe-{name}.txt';path.write_bytes(text.encode('latin1'))
  got=run('recipes',path);assert got.splitlines()[0]==expect,(name,got)
  if name=='good':assert 'LOOKUP table=1 builtin_inf=1 builtin_speeder=1 abilities=0 none=0' in got,got
 report['recipeCases']=len(recipe_cases)

 # 5. Faction tint metadata from raw indexed art.
 tint_cases=0
 for raw,intensity,size,expect in [(20,3,(436,758),f'TINT ok=1 rgb={",".join(map(str,g(30)))}'),(250,0,(436,758),f'TINT ok=1 rgb={",".join(map(str,g(4)))}'),
   (255,3,(436,758),'TINT ok=0 rgb=7,7,7'),(20,255,(436,758),'TINT ok=0 rgb=7,7,7'),(20,3,(435,758),'TINT ok=0 rgb=7,7,7'),(20,3,(436,757),'TINT ok=0 rgb=7,7,7'),(0,0,(1024,768),f'TINT ok=1 rgb={",".join(map(str,g(10)))}')]:
  w,h=size;img=bytearray([9])*(w*h)
  if w>435 and h>745:img[745*w+435]=raw
  if w>161 and h>757:img[757*w+161]=intensity
  path=out/f'tint-{tint_cases}.raw';path.write_bytes(bytes(img))
  got=run('tint',path,w,h,gpath).strip();assert got==expect,(raw,intensity,size,got,expect);tint_cases+=1
 report['tintCases']=tint_cases

 # 6. Design-keyed vehicle set: reuse, missing-asset memo, abilities, eviction.
 d=out/'set';d.mkdir();(d/'vi.cvr').write_bytes(model(RED,frames=15));(d/'viptr00.cvr').write_bytes(model(BLUE))
 (d/'cvr_units.txt').write_text(''.join(f'{20+k} 0 0 1 1.25 -400 15 100000 : vi viptr00\n' for k in range(7)))
 got=run('set',d).strip()
 assert got=='SET first=1 same=1 reads=2 missing=0 memo=1 ability=0 table=1 evicted_reload=1',got
 report['setControls']=6
 (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print('CVR VEHICLE',json.dumps(report),'PASS')
if __name__=='__main__':main()
