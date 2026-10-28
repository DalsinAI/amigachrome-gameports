# SPDX-License-Identifier: GPL-3.0-or-later
"""Map overlay, minimap, message log and base window controls on synthetic
maps, art sheets and saves. No original game data is read or embedded."""
import argparse,json,os,re,struct,subprocess
from pathlib import Path
from test_original_world import valid_fixture
from test_original_game import decode
from test_cvr_infantry import model as cvr_model
from test_cvr_palette import table as faction_table

ROAD,TUBE,MINE,FUNGUS,SOLAR,RIVER,BUNKER,MONOLITH,FARM,AIRBASE,SOIL,FOREST,COND,MIRROR,BORE,POD,SENSOR,BASE=0x4,0x8,0x10,0x20,0x40,0x80,0x800,0x2000,0x8000,0x40000,0x80000,0x200000,0x400000,0x800000,0x1000000,0x10000000,0x80000000,0x1
IMPROVEMENTS=[SOIL,MINE,SOLAR,COND,MIRROR,BORE,SENSOR,BUNKER,AIRBASE,MONOLITH,POD]
TER1={SOIL:((822,453),(822,453)),MINE:((607,64),(506,64)),SOLAR:((607,127),(506,127)),COND:((506,253),)*2,MIRROR:((607,253),)*2,BORE:((708,253),)*2,
 SENSOR:((708,316),)*2,BUNKER:((506,316),)*2,AIRBASE:((607,316),)*2,MONOLITH:((304,0),)*2,POD:((519,379),(418,379))}
RIVER_RGB,ROAD_RGB,TUBE_CORE,MINE_RGB=(56,104,208),(150,112,64),(64,96,120),(150,150,150)
FACTIONS=[(0,0,0),(230,60,60),(60,200,90),(70,110,240),(230,210,60),(200,90,220),(60,210,220),(240,150,60)]

# Golden frame digests: identical on the sanitizer host and big-endian 68040.
GOLDEN={
 'basewin-forecast-other-viewer':'f1f06d59',
 'basewin-saved-owner':'7bcdac41',
 'messages-0':'be558aed',
 'messages-1':'04aaa7e7',
 'messages-2':'817299e5',
 'messages-3':'2b9b047d',
 'minimap-omniscient':'f237c16d',
 'minimap-small':'eb67d263',
 'minimap-viewer2':'62d57b2d',
 'overlay-flat-edge':'7de5f305',
 'overlay-layer-1-v-1':'2498a8d4',
 'overlay-layer-1-v2':'2498a8d4',
 'overlay-layer-16-v-1':'61c1377d',
 'overlay-layer-16-v2':'53783451',
 'overlay-layer-2-v-1':'b64ef4e0',
 'overlay-layer-2-v2':'b64ef4e0',
 'overlay-layer-32-v-1':'6add85c5',
 'overlay-layer-32-v2':'2b003c05',
 'overlay-layer-4-v-1':'87df222d',
 'overlay-layer-4-v2':'04ce1065',
 'overlay-layer-8-v-1':'c3793fb5',
 'overlay-layer-8-v2':'c3793fb5',
 'overlay-omniscient-glyphs':'59a18fb0',
 'overlay-viewer2-art':'fd2894c6',
 'overlay-viewer2-art-z1':'5d24654f',
 'overlay-wrap-seam':'fba42a56',
}

class Tile:
 __slots__=('climate','val3','owner','vis','items','seen')
 def __init__(s):s.climate=0x68;s.val3=0;s.owner=-1;s.vis=0;s.items=0;s.seen=[0]*7

def make_map(w=24,h=16,flat=False):
 tiles={}
 for y in range(h):
  for x in range(w):
   if (x+y)&1:continue
   t=Tile();tiles[(x,y)]=t
   if x<4:t.climate=0x40 if x>=2 else 0x20
   if x in (20,21):t.val3=0x80
   if 8<=x<=13:t.owner=1
   elif 16<=x<=19:t.owner=3
   t.vis=0xff if x<16 else 0x01
 def add(x,y,bits):
  t=tiles.get((x%w,y))
  if t:t.items|=bits
 for k in range(6):add(6+k,4+k,RIVER)        # diagonal river reaching ...
 add(5,3,0);add(3,1,0)
 for k in range(7):add(6+2*k,12,ROAD)         # horizontal road (corner links)
 add(20,12,BASE)
 for k in range(3):add(14+k,2+k,TUBE)
 add(8,2,MINE);add(10,2,SOLAR);add(12,6,SENSOR);add(9,9,FOREST);add(11,9,FUNGUS);add(13,9,FARM|SOIL)
 add(15,9,COND);add(17,9,MIRROR);add(19,9,BORE);add(21,9,BUNKER);add(23,9,AIRBASE);add(1,9,FUNGUS|POD);add(7,15,MONOLITH)
 add(2,14,MINE)  # sea mine
 for t in tiles.values():
  t.seen=[t.items]*7;t.seen[1]=t.items&~MINE   # faction 2 last saw no mines
 return dict(w=w,h=h,flat=flat,tiles=tiles)

def write_map(path,m):
 b=bytearray(struct.pack('<IIB',m['w'],m['h'],int(m['flat'])))
 for y in range(m['h']):
  for x in range(m['w']):
   if (x+y)&1:continue
   t=m['tiles'][(x,y)];b+=struct.pack('<BBbB8I',t.climate,t.val3,t.owner,t.vis,t.items,*t.seen)
 path.write_bytes(bytes(b))

def pcx(w,h,pixel,palette):
 stride=(w+1)&~1;header=bytearray(128);header[0:4]=bytes([10,5,1,8]);struct.pack_into('<4H',header,4,0,0,w-1,h-1);header[65]=1;struct.pack_into('<H',header,66,stride)
 body=bytearray()
 for y in range(h):
  for x in range(stride):
   v=pixel(x,y) if x<w else 0
   body.extend(bytes([0xc1,v]) if v>=192 else bytes([v]))
 return bytes(header+body+b'\x0c'+bytes(palette))
TEX_PAL=bytes([125,0,128])+bytes(v for i in range(1,256) for v in (i,(i*3)&255,(255-i)))
TER_PAL=bytes([255,0,255])+bytes(v for i in range(1,256) for v in ((i*5)&255,i,(i*11)&255))
def tex_index(x,y):return 0 if (x+y)%7==0 else ((x//3+y//5)%250)+1
def ter_index(x,y):return 0 if (x*y)%5==0 else ((x+2*y)%200)+20
def tex_rgb(x,y):i=tex_index(x,y);return tuple(TEX_PAL[i*3:i*3+3])
def ter_rgb(x,y):i=ter_index(x,y);return tuple(TER_PAL[i*3:i*3+3])

# ---- independent oracle of the overlay rules ----------------------------------
def tile_at(m,x,y):
 if y<0 or y>=m['h']:return None
 if not m['flat']:x%=m['w']
 if x<0 or x>=m['w'] or (x+y)&1:return None
 return m['tiles'][(x,y)]
def explored(t,viewer):return viewer<0 or viewer>7 or (t.vis>>viewer)&1
def tile_items(t,viewer):return t.seen[viewer-1] if 1<=viewer<=7 else t.items
def ocean(t):return (t.climate&0xe0)<0x60
def position(m,cam,x,y):
 dx=x-cam['x'];w=m['w']
 if not m['flat']:
  if dx>w//2:dx-=w
  elif dx< -(w//2):dx+=w
 sx=cam['W']//2+dx*32*cam['z'];sy=cam['H']//2+(y-cam['y'])*16*cam['z']
 return sx,sy,(-64*cam['z']<=sx<cam['W']+64*cam['z'] and -64*cam['z']<=sy<cam['H']+64*cam['z'])
def visit(m,cam):
 z=cam['z'];rx=cam['W']//(64*z)+3;ry=cam['H']//(32*z)+3;xlo,xhi=cam['x']-rx,cam['x']+rx
 if not m['flat'] and 2*rx+1>m['w']:xlo=cam['x']-m['w']//2;xhi=xlo+m['w']-1
 for y in range(cam['y']-ry,cam['y']+ry+1):
  for raw in range(xlo,xhi+1):
   t=tile_at(m,raw,y)
   if t is None:continue
   sx,sy,ok=position(m,cam,raw,y)
   if ok:yield raw,y,t,sx,sy
EDGES=[(1,-1),(1,1),(-1,1),(-1,-1)];ALL=[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]
def expected_stats(m,cam,viewer,layers,texture,ter1):
 s=dict.fromkeys('tiles unexplored forest fungus farms rivers river_links roads road_links tubes tube_links mines solar sensors other borders art glyphs'.split(),0)
 for raw,y,t,sx,sy in visit(m,cam):
  s['tiles']+=1
  if not explored(t,viewer):
   if layers&32:s['unexplored']+=1
   continue
  it=tile_items(t,viewer);oc=ocean(t)
  if layers&1:
   if it&FARM and not oc:s['farms']+=1;s['art' if texture else 'glyphs']+=1
   if it&FOREST:s['forest']+=1;s['art' if texture else 'glyphs']+=1
   if it&FUNGUS:s['fungus']+=1;s['art' if texture else 'glyphs']+=1
  if layers&2 and it&RIVER and not oc:
   s['rivers']+=1
   for dx,dy in EDGES:
    n=tile_at(m,raw+dx,y+dy)
    if n and explored(n,viewer) and (tile_items(n,viewer)&RIVER or ocean(n)):s['river_links']+=1
  if layers&4 and it&(ROAD|TUBE|BASE):
   tube=bool(it&TUBE);s['tubes' if tube else 'roads']+=1
   for dx,dy in ALL:
    n=tile_at(m,raw+dx,y+dy)
    if not n or not explored(n,viewer):continue
    ni=tile_items(n,viewer)
    if tube and ni&(TUBE|BASE):s['tube_links']+=1
    elif ni&(ROAD|TUBE|BASE) and (not it&BASE or ni&(ROAD|TUBE)):s['road_links']+=1
  if layers&8:
   for bit in IMPROVEMENTS:
    if not it&bit:continue
    key={MINE:'mines',SOLAR:'solar',SENSOR:'sensors'}.get(bit,'other');s[key]+=1
    s['art' if ter1 else 'glyphs']+=1
  if layers&16 and 0<t.owner<8:
   for dx,dy in EDGES:
    n=tile_at(m,raw+dx,y+dy)
    if not(n and n.owner==t.owner):s['borders']+=1
 return s
def variation(x,y):return (x*7+y*3)&15

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--print-golden',action='store_true');a=ap.parse_args()
 out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 exe=out/'control'
 subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,*[str(src/(n+'.cpp')) for n in ['original_map','original_game','original_faction','original_world','pcx_image','cvr_model','cvr_raster','cvr_lighting','cvr_palette','cvr_vehicle','map_overlays','ui_panel','ui_minimap','ui_messages','ui_basewin','ui_mapview','ui_render_control']],'-o',str(exe)],check=True)
 def run(*args):
  cmd=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(exe),*map(str,args)]
  p=subprocess.run(cmd,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True,errors='replace')
  assert p.returncode==0,(args,p.returncode,p.stdout,p.stderr);return p.stdout
 digests={};report={}
 def golden(name,digest):
  digests[name]=digest
  if not a.print_golden:assert GOLDEN.get(name)==digest,(name,digest,GOLDEN.get(name))

 # ---- overlays ----
 texture=out/'texture.pcx';texture.write_bytes(pcx(1024,768,tex_index,TEX_PAL))
 ter1=out/'ter1.pcx';ter1.write_bytes(pcx(1024,768,ter_index,TER_PAL))
 maps={'round':make_map(),'flat':make_map(flat=True)}
 for name,m in maps.items():write_map(out/f'{name}.map',m)
 runs=[('omniscient-glyphs','round',-1,63,1,12,8,640,400,False,False),('viewer2-art','round',2,63,2,12,8,480,320,True,True),
  ('viewer2-art-z1','round',2,63,1,14,10,512,256,True,True),('flat-edge','flat',-1,63,2,1,1,400,300,True,False),
  ('wrap-seam','round',-1,63,2,23,9,384,256,False,True)]
 for viewer in [-1,2]:
  for layer in [1,2,4,8,16,32]:runs.append((f'layer-{layer}-v{viewer}','round',viewer,layer,2,12,8,480,320,True,False))
 for name,mname,viewer,layers,z,cx,cy,W,H,tex,t1 in runs:
  m=maps[mname];cam=dict(x=cx,y=cy,z=z,W=W,H=H)
  probes=[]
  for raw,y,t,sx,sy in visit(m,cam):
   if 0<=sx<W and 0<=sy<H:probes.append((raw,y,t,sx,sy))
  text=run('overlays',out/f'{mname}.map',texture if tex else '-',ter1 if t1 else '-',viewer,layers,z,cx,cy,W,H,out/f'{name}.ppm',*[f'{sx},{sy}' for *_,sx,sy in probes])
  st=dict(re.findall(r'(\w+)=(\d+)',text.splitlines()[0]));st={k:int(v) for k,v in st.items()}
  st['art']=st.pop('art');st['other']=st.pop('other')
  want=expected_stats(m,cam,viewer,layers,tex,t1)
  assert st==want,(name,st,want)
  colours={tuple(map(int,p.split()[1].split(','))):tuple(map(int,p.split()[2].split(','))) for p in text.splitlines() if p.startswith('PROBE')}
  checked=0
  for raw,y,t,sx,sy in probes:
   got=colours[(sx,sy)];it=tile_items(t,viewer) if explored(t,viewer) else 0
   if layers&32 and not explored(t,viewer):assert got==(0,0,0),(name,raw,y,got);checked+=1
   elif layers==2 and it&RIVER and not ocean(t):assert got==RIVER_RGB,(name,raw,y,got);checked+=1
   elif layers==4 and it&ROAD and not it&(TUBE|BASE):assert got==ROAD_RGB,(name,raw,y,got);checked+=1
   elif layers==4 and it&TUBE:assert got==TUBE_CORE,(name,raw,y,got);checked+=1
   elif layers==8 and it==MINE:assert got==MINE_RGB,(name,raw,y,got);checked+=1
   elif layers==1 and tex and it&FOREST and not it&(FUNGUS|FARM):
    v=variation(raw,y);want_rgb=tex_rgb(526+57*(v&3)+28,6+57*(v>>2)+28)
    if want_rgb!=(125,0,128):assert got==want_rgb,(name,raw,y,got,want_rgb);checked+=1
   elif layers==63 and t1 and z==2 and explored(t,viewer) and bin(it).count('1')==1 and it in TER1 and not t.owner>0:
    ax,ay=TER1[it][1 if ocean(t) else 0];want_rgb=ter_rgb(ax+50,ay+25)
    if want_rgb!=(255,0,255):assert got==want_rgb,(name,raw,y,got,want_rgb);checked+=1
  golden('overlay-'+name,text.split('DIGEST ')[1].split()[0])
  report.setdefault('overlayProbes',0);report['overlayProbes']+=checked
 report['overlayRuns']=len(runs)
 # A frame larger than the per-call budget is rejected as a whole.
 assert 'OVERLAYS rejected' in run('overlays',out/'round.map','-','-',-1,63,1,12,8,4097,10)
 # ---- minimap ----
 m=maps['round']
 marks='12:8:1:1,16:4:3:0,30:30:2:1,6:2:9:0,18:2:3:1'
 mm=[('omniscient',-1,96,64,12,8),('viewer2',2,96,64,12,8),('small',2,24,16,23,1)]
 for name,viewer,rw,rh,cx,cy in mm:
  probe=[(10+px,10+py) for py in range(0,rh,max(1,rh//8)) for px in range(0,rw,max(1,rw//8))]
  text=run('minimap',out/'round.map',viewer,rw,rh,cx,cy,marks,*[f'{x},{y}' for x,y in probe],'5,5')
  head=dict(re.findall(r'(\w+)=(\w+)',text.splitlines()[0]))
  explored_count=0;owned=0
  for py in range(rh):
   for px in range(rw):
    y=py*m['h']//rh;x=px*m['w']//rw
    if (x+y)&1:x=x+1 if x+1<m['w'] else x-1
    t=tile_at(m,x,y)
    if explored(t,viewer):
     explored_count+=1
     if 0<t.owner<8 and not ocean(t):owned+=1
  assert int(head['pixels'])==rw*rh and int(head['explored'])==explored_count and int(head['owned'])==owned,(name,head,explored_count,owned)
  vis_bases=sum(1 for x,y,f,b in [(12,8,1,1),(18,2,3,1)] if explored(tile_at(m,x,y),viewer))
  assert int(head['bases'])==vis_bases,(name,head)
  for line in text.splitlines()[1:]:
   x,y=map(int,line.split()[1].split(','));hit=line.split('map=')[1]
   if (x,y)==(5,5):assert hit.startswith('0'),line;continue
   px,py=x-10,y-10;ty=py*m['h']//rh;tx=px*m['w']//rw
   if (tx+ty)&1:tx=tx+1 if tx+1<m['w'] else tx-1
   assert hit==f'1 {tx},{ty}',(line,tx,ty)
  golden('minimap-'+name,head['digest'])
 report['minimapRuns']=len(mm)
 # ---- message log ----
 script=['add 1 0 Turn one begins','add 1 0 Turn one begins','add 1 3 Combat\x07report','add 2 9 bad kind','add 2 2 ','show 300 120','newest 0','newest 1',
  *[f'add {3+i} 1 Message {i}' for i in range(70)],'newest 0','newest 63','newest 64','scroll 5','show 300 120','scroll 1000','scroll -3','add 99 4 Arrived while scrolled','scroll -1000','show 300 200','clear','show 300 120']
 sp=out/'messages.txt';sp.write_bytes('\n'.join(script).encode('latin1'))
 text=run('messages',sp).splitlines()
 adds=[l for l in text if l.startswith('ADD')]
 assert adds[:5]==['ADD 1 count=1 total=1','ADD 1 count=1 total=2','ADD 1 count=2 total=3','ADD 0 count=2 total=3','ADD 0 count=2 total=3'],adds[:5]
 assert adds[-2]=='ADD 1 count=64 total=73' and adds[-1]=='ADD 1 count=64 total=74',adds[-2:]
 newest=[l for l in text if l.startswith('NEWEST')]
 assert newest==['NEWEST T1 k3 x1 |Combat?report|','NEWEST T1 k0 x2 |Turn one begins|','NEWEST T72 k1 x1 |Message 69|','NEWEST T9 k1 x1 |Message 6|','NEWEST none'],newest
 offsets=[l for l in text if l.startswith('OFFSET')]
 assert offsets==['OFFSET 5','OFFSET 63','OFFSET 60','OFFSET 0'],offsets
 panels=[i for i,l in enumerate(text) if l.startswith('PANEL')]
 assert len(panels)==4
 first=text[panels[0]+1:panels[0]+4]
 assert first==['TEXT 12 22 284 3 1 |MESSAGES (2)|','TEXT 14 94 276 0 0 |T1  Turn one begins (x2)|','TEXT 14 112 276 0 0 |T1  Combat?report|'],first
 assert text[panels[3]].startswith('PANEL items=1 ')
 for i,p in enumerate(panels):golden(f'messages-{i}',text[p].split('digest=')[1])
 report['messageChecks']=len(adds)+len(newest)+len(offsets)+len(panels)
 # ---- base window ----
 save=bytearray(valid_fixture(12,vehicles=3,bases=2));_,off,_=decode(bytes(save))
 b0=off['bases']
 name=b'Synthetic Base\0';save[b0+0x13:b0+0x13+len(name)]=name
 struct.pack_into('<hh',save,b0,6,4);save[b0+4]=1;save[b0+6]=5
 struct.pack_into('<i',save,b0+0x4c,2);struct.pack_into('<3i',save,b0+0x50,320,-2,-6)
 worked=(1<<0)|(1<<1)|(1<<4)|(1<<9)|(1<<20);struct.pack_into('<i',save,b0+0x78,worked);struct.pack_into('<i',save,b0+0x7c,1)
 fac=bytearray(12);fac[0]=0b1010;fac[1]=0b10;fac[11]=0x80;save[b0+0x8c:b0+0x98]=fac
 struct.pack_into('<3i',save,b0+0xc0,12,9,7);struct.pack_into('<3i',save,b0+0xd0,2,3,4);struct.pack_into('<3i',save,b0+0xe0,0,0,3)
 struct.pack_into('<3i',save,b0+0xf0,10,6,0);struct.pack_into('<3i',save,b0+0x100,5,6,7);struct.pack_into('<3i',save,b0+0x118,2,1,0)
 struct.pack_into('<ii',save,b0+0x3c,17,23)
 proto=off['prototypes']+320*52;pname=b'Scout Patrol\0';save[proto:proto+len(pname)]=pname
 sp=out/'base.sav';sp.write_bytes(bytes(save))
 info,_,spans=decode(bytes(save));W=int(info['width']);H=int(info['height']);tiles=spans['tiles']
 def stile(x,y):
  i=(y*W+x)//2;p=tiles[i*44:(i+1)*44];return dict(climate=p[0],vis=p[4],val3=p[5],owner=p[7],items=struct.unpack_from('<I',p,8)[0],seen=struct.unpack_from('<7I',p,16))
 dx=[0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1];dy=[0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3]
 names=['Headquarters','Childrens Creche','Recycling Tanks','Perimeter Defense','Tachyon Field','Recreation Commons','Energy Bank','Network Node','Biology Lab','Skunkworks']
 cases=[('saved-owner',-1,'-'),('forecast-other-viewer',3,'14,10,4,8,5,3,9,2,7')]
 for cname,viewer,fc in cases:
  text=run('basewin',sp,0,viewer,fc,out/f'base-{cname}.ppm',*names).splitlines()
  model=dict(re.findall(r'(\w+)=(\|[^|]*\||[-\d,]+)',text[0]))
  assert model['name']=='|Synthetic Base|' and model['pop']=='5' and model['faction']=='1' and model['queue']=='3' and model['facilities']=='4',model
  present_worked=sum(1 for i in range(21) if (worked>>i)&1 and 0<=4+dy[i]<H)
  assert model['worked']==str(present_worked) and present_worked>=3,(model,present_worked,H)
  if fc=='-':assert model['fresh']=='0' and model['n']=='12,10,2' and model['m']=='9,6,3' and model['e']=='7,3,4',model
  else:assert model['fresh']=='1' and model['n']=='14,10,4' and model['m']=='8,5,3' and model['e']=='9,2,7',model
  tl=[l for l in text if l.startswith('TILE')];assert len(tl)==21
  for i,l in enumerate(tl):
   x=(6+dx[i])%W;y=4+dy[i];parts=l.split()
   assert parts[2]==f'{x},{y}',(l,x,y)
   if 0<=y<H:
    t=stile(x,y);oc=(t['climate']&0xe0)<0x60;exp=viewer<0 or viewer==1 or (t['vis']>>viewer)&1
    it=t['items'] if (viewer<0 or viewer==1) else (t['seen'][viewer-1] if 1<=viewer<=7 else t['items'])
    want=f"p1 w{(worked>>i)&1} o{int(oc)} r{int((not oc) and (t['val3']&0xc0)>=0x80)} f{int(bool(it&FUNGUS))} e{int(bool(exp))}"
    assert ' '.join(parts[3:])==want,(l,want)
   else:assert parts[3]=='p0',l
  assert [l for l in text if l.startswith('QUEUE')]==['QUEUE |Scout Patrol|','QUEUE |Childrens Creche|','QUEUE |Recreation Commons|'],text
  assert [l for l in text if l.startswith('FACILITY')]==['FACILITY |Headquarters|','FACILITY |Recycling Tanks|','FACILITY |Biology Lab|','FACILITY |Facility 95|'],text
  render=[l for l in text if l.startswith('RENDER')][0]
  items=[l for l in text if l.startswith('TEXT')]
  assert any('|Synthetic Base|' in l for l in items) and any('|> Scout Patrol|' in l for l in items) and any('|2. Recreation Commons|' in l for l in items),items
  golden('basewin-'+cname,render.split('digest=')[1])
 assert 'BASEWIN rejected' in run('basewin',sp,2,-1,'-','-')
 report['baseWindowCases']=len(cases)+1
 # ---- map view glue: owner assets, faction colours, assembled vehicles ----
 save=bytearray(valid_fixture(7,vehicles=3,bases=2));_,off,_=decode(bytes(save))
 for f in range(8):struct.pack_into('<i',save,off['faction_definitions']+f*0x59c+0x508,0)
 for i,(chassis,weapon,armor,reactor) in enumerate([(0,0,0,1),(9,0,0,1),(1,0,0,1)]):
  pp=off['prototypes']+(320+i)*52;struct.pack_into('<I4B',save,pp+0x20,0,chassis,weapon,armor,reactor)
  vp=off['vehicles']+i*52;save[vp+0xe]=1;save[vp+0x29]=0
 vsave=out/'view.sav';vsave.write_bytes(bytes(save))
 P=bytes((i*13+7)%256 for i in range(768))
 g=bytearray(768)
 for i in range(8):
  for c in range(3):g[i*3+c]=192 if i==7 else (128 if i&(1<<c) else 0);g[(248+i)*3+c]=128 if i==0 else (255 if i&(1<<c) else 0)
 g[24]=8;g[27]=9;g[246*3]=245;g[247*3]=246
 for i in range(708):g[30+i]=min(P[i],235)
 raw=100;tint=tuple(g[((raw+10)&255)*3:((raw+10)&255)*3+3])
 painted=faction_table(bytes(g),bytes(range(256))*24,tint)[5]
 for variant in ['palette','model']:
  d=out/f'view-{variant}';d.mkdir()
  if variant=='palette':(d/'palette.pcx').write_bytes(pcx(4,4,lambda x,y:x+4*y,P))
  art=lambda x,y:raw if (x,y)==(435,745) else (3 if (x,y)==(161,757) else 9)
  (d/'faction1.pcx').write_bytes(pcx(1024,768,art,TEX_PAL))
  (d/'vi.cvr').write_bytes(cvr_model((255,0,0),frames=15));(d/'viptr00.cvr').write_bytes(cvr_model((255,0,0),offset=(100,0,0)))
  (d/'vb.cvr').write_bytes(cvr_model((0,255,0),frames=1));(d/'vbp.cvr').write_bytes(cvr_model((0,0,255),frames=1,offset=(0,150,0)))
  (d/'cvr_units.txt').write_text('9 0 0 1 1.0 0 1 1000 : vb vbp*\n')
  text=run('mapview',d,vsave).splitlines()
  logs=[l for l in text if l.startswith('LOG')]
  if variant=='palette':assert logs[0]=='LOG k0 |Faction unit colours ready for 1 factions.|',logs
  else:assert logs[0]=='LOG k0 |palette.pcx not found: units keep model colours.|',logs
  assert logs[1:]==['LOG k0 |cvr_units.txt: 1 extra unit recipes.|','LOG k0 |ter1.pcx not found: improvements use simple markers.|'],logs
  vehicles={int(l.split()[1]):dict(re.findall(r'(\w+)=([\d,;]*)',l)) for l in text if l.startswith('VEHICLE')}
  def rgbs(v):return {tuple(map(int,c.split(','))) for c in v['colours'].split(';') if c}
  assert vehicles[0]['model']=='1' and vehicles[0]['drawn']=='1' and int(vehicles[0]['changed'])>0,vehicles
  assert vehicles[1]['model']=='1' and vehicles[1]['drawn']=='1',vehicles
  assert vehicles[2]['model']=='0' and vehicles[2]['drawn']=='0',vehicles   # speeder parts absent
  if variant=='palette':assert rgbs(vehicles[0])=={(128,0,128)} and rgbs(vehicles[1])=={(128,0,128),tuple(g[painted*3:painted*3+3])},(vehicles,painted)
  else:assert rgbs(vehicles[0])=={(255,0,0)} and rgbs(vehicles[1])=={(0,255,0),(0,0,255)},vehicles
  assert 'OVERLAYS 1' in text and text[[i for i,l in enumerate(text) if l.startswith('MINIMAP')][0]].startswith('MINIMAP 1 ')
  assert [l for l in text if l.startswith('CLICK')][0].startswith('CLICK 1 '),text
  assert int([l for l in text if l.startswith('BASEWIN')][0].split('=')[1])>=10 and int([l for l in text if l.startswith('MESSAGES')][0].split('=')[1])>=4
  assert [l for l in text if l.startswith('VIEWER -1')]==['VIEWER -1 bases_visible=2'],text
  report.setdefault('mapViewVariants',0);report['mapViewVariants']+=1
 report['paintedFactionIndex']=painted
 report['goldenDigests']=len(digests)
 if a.print_golden:print(json.dumps(digests,indent=1,sort_keys=True))
 (out/'receipt.json').write_text(json.dumps(dict(report=report,digests=digests),indent=2)+'\n');print('MAP RENDER',json.dumps(report),'PASS')
if __name__=='__main__':main()
