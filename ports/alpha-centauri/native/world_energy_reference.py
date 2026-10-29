# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent current-wire convoy context for the full income controls."""
import struct
import test_world_fresh_minerals as minerals
from test_world_tile_yields import ROW
from test_tile_yields import expected as leaf
from tile_bonus_reference import bonus_model
from test_base_nutrients import expected as nutrient

def resource_model(profile,text,data):
 intake=minerals.convoy.intake;info,o,sp=intake.decode(data);v=int(info['version']);legacy=v<11;bs=304 if legacy else 308;fs=0x20bc if legacy else 0x20cc;schema=intake.FACTIONS['LegacyFactionV10' if legacy else 'Faction'];b=sp['bases'];owner=b[4];pref=sp['legacy'] if legacy else sp['preferences'];shift=0x7c if legacy else 0;tables=intake.sections(text);width,height=int(info['width']),int(info['height']);count=int(info['bases']);pf=lambda off:struct.unpack_from('<i',pref,off-(shift if off>=0x184 else 0))[0]
 fail=lambda e:[e]+[0x5a5a5a5a]*130
 state=lambda name:intake.scalar(sp['factions'][owner*fs:(owner+1)*fs],schema,name)
 projects=list(struct.unpack_from('<'+('33i' if legacy else '64i'),pref,0x84))+([-1]*31 if legacy else [])
 keys={r[1].lower():i for i,r in enumerate(tables['TECHNOLOGY'])};keys.update(none=-1,disable=-2)
 def basic(name):
  row,col,limits=ROW[name];value=tables['RULES'][row][col];return keys[value.lower()] if limits is None else max(limits[0],min(limits[1],int(value)))
 def has(id):
  if owner==0:return False
  if id==-1:return True
  if not 0<=id<88:return False
  a,z=[keys[k.lower()] for k in tables['TECHNOLOGY'][id][6:8]];return a!=-2 and (z!=-2 or a==-1) and bool(pref[0x1e0-shift+id]&(1<<owner))
 def project(index):
  at=projects[index]
  if at>=count:raise ValueError('project')
  return at>=0 and b[at*bs+4]==owner
 def tile(x,y):return sp['tiles'][((y*width+x)//2)*44:((y*width+x)//2+1)*44]
 effect_error,effects=intake.effect_model(intake.effect_facts(text,data)[owner])
 if effect_error:return fail(7 if effect_error==2 else 1)
 to=[0]*4;fr=[0]*4;harvesters=reduction=energy=0
 for i in range(int(info['vehicles'])):
  vehicle=sp['vehicles'][52*i:52*(i+1)];unit=struct.unpack_from('<h',vehicle,10)[0];proto=sp['prototypes'][52*unit:52*(unit+1)]
  if vehicle[14]!=owner or proto[42]!=10 or vehicle[17]!=3:continue
  channel=vehicle[38]
  if channel>=4:return fail(1)
  home=struct.unpack_from('<h',vehicle,46)[0];x,y=struct.unpack_from('<hh',vehicle);sq=tile(x,y);items,landmarks=struct.unpack_from('<II',sq,8)
  if home==0:
   if items&1 and sq[2]&15<8:fr[channel]+=1;continue
   harvesters+=1
   if channel==3:reduction=0;continue
   row=[0]*59;row[0]=int(profile=='expansion');row[1:7]=[sq[0],sq[2],sq[5],items,landmarks,bonus_model(struct.unpack_from('<I',sp['metadata'],8)[0],x,y,pf(0xc),sq[0],items)];row[7]=int(profile=='expansion');row[9]=state('SE_planet_pending');row[10]=state('SE_economy_pending');row[11:14]=effects[1:4];row[14]=state('base_count');row[15]=int(bool(struct.unpack_from('<I',b,0x30)[0]&4));row[18],row[19]=struct.unpack_from('<II',b,0x8c)
   try:row[16]=int(profile=='expansion' and project(33))
   except ValueError:return fail(1)
   row[17]=int(bool(struct.unpack_from('<I',sp['faction_definitions'],owner*0x59c+0x508)[0]&0x100))
   for j,name in enumerate(['tech_three_nutrients_sqr','tech_three_minerals_sqr','tech_three_energy_sqr','tech_mining_platform_bonus']):row[20]|=int(has(basic(name)))<<j
   merchant=projects[3]
   if merchant>=count:return fail(1)
   row[21]=int(pf(0x184)==owner);row[22]=int(merchant==0)
   # Use schema-derived preference positions below; do not infer layout from names.
   def preference(name):
    field=next(x for x in intake.SCHEMA['GamePrefs']['fields'] if x['name']==name);off=field['offset']-(shift if field['offset']>=0x184 else 0);return struct.unpack_from('<'+intake.FORMATS[field['type']],pref,off)[0]
   row[23]=int(preference('DustCloudDuration')!=0);row[24]=preference('SolarFlaresEvent')&0xffffffff;row[27]=basic('tgl_nutrient_effect_with_mine');row[28]=basic('limit_mineral_mine_sans_road');row[29]=struct.unpack_from('<I',b,0x34)[0];row[30]=reduction;row[31]=energy
   for j,resource in enumerate(tables['RESOURCEINFO']):row[32+3*j:35+3*j]=[0 if z=='*' else int(z) for z in resource[1:4]]
   mirrors=not(items&(0x2000|0x1000000)) and not(items&0x20 and sq[0]&0xe0>0x3f) and sq[0]&0xe0>=0x60 and not(items&0x200000) and bool(items&0x40)
   if mirrors:
    for dx,dy in [(0,-2),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1)]:
     nx,ny=x+dx,y+dy
     if not struct.unpack_from('<I',sp['metadata'],28)[0]:nx%=width
     if 0<=nx<width and 0<=ny<height:
      near=tile(nx,ny);row[26]+=int(bool(struct.unpack_from('<I',near,8)[0]&0x800000) and struct.unpack_from('<b',near,7)[0]==owner)
   answer=leaf(row)
   if answer[0]:return fail(7 if answer[0]==2 else 1)
   to[channel]+=answer[1+channel];reduction+=answer[6+channel]
   if channel==2:energy=answer[5]
   if any(not -(1<<31)<=n<(1<<31) for n in [*to,reduction]):return fail(7)
  elif home>=0 and (x,y)==struct.unpack_from('<hh',b):to[channel]+=1
 fresh=intake.model(profile,text,data,0,expansion_enabled=profile=='expansion',reduction_before=reduction,energy_before=energy)
 if fresh[0]:return fail(fresh[0])
 bits=int(bool(b[0x8c]&4))|2*int(bool(struct.unpack_from('<I',b,0x30)[0]&4));nut=nutrient([struct.unpack_from('<b',b,6)[0],state('SE_growth_pending'),fresh[7],to[0],fr[0],basic('nutrient_req_citizen'),struct.unpack_from('<i',b,0x3c)[0],state('nutrient_surplus_total'),0,bits])
 if nut[0]:return fail(7 if nut[0]==3 else 1)
 food=[0,*to,*fr,harvesters,reduction,energy,*fresh[1:],*nut[1:]]
 return minerals.model(profile,text,data,1,food_override=food)
