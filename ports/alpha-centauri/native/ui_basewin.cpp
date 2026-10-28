/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in ui_basewin.h. */
#include "ui_basewin.h"
#include "map_overlays.h"
#include <stdio.h>
#include <string.h>
namespace alpha { namespace {
const int32_t radius_dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t radius_dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
void copy_text(char* out,size_t n,const char* in){size_t i=0;for(;in[i] && i+1<n;++i){const unsigned char c=(unsigned char)in[i];out[i]=(c<32 || c==127)?'?':(char)c;}out[i]=0;}
void put(const UiFrame& f,int x,int y,const uint8_t* rgb){if(x<0 || y<0 || x>=(int)f.width || y>=(int)f.height)return;uint8_t* p=f.rgb+((size_t)y*f.width+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
void small_diamond(const UiFrame& f,int cx,int cy,int hw,int hh,const uint8_t* fill,const uint8_t* edge){
 for(int dy=-hh;dy<=hh;++dy){const int span=hw*(hh-(dy<0?-dy:dy))/hh;for(int dx=-span;dx<=span;++dx)put(f,cx+dx,cy+dy,(edge && (dx==-span || dx==span || dy==-hh || dy==hh))?edge:fill);}
}
}
bool build_base_window(const OriginalWorld& w,size_t index,const FacilityDefinitions* defs,const BaseWindowForecast* forecast,int viewer,BaseWindowModel& out){
 const OriginalBase* b=w.base(index);if(!b)return false;
 BaseWindowModel m;memset(&m,0,sizeof(m));
 copy_text(m.name,sizeof(m.name),b->name);
 m.population=b->pop_size;m.faction=b->faction_id;m.specialists=b->specialist_total;m.talents=b->talent_total;m.drones=b->drone_total;
 const auto& info=w.game().map.info;
 MapTiles tiles{info.width,info.height,info.flat,w.tile(0),info.tile_count};
 const bool own=viewer<0 || viewer==b->faction_id;
 for(unsigned i=0;i<21;++i){BaseWindowTile& t=m.tiles[i];t.x=b->x+radius_dx[i];t.y=b->y+radius_dy[i];
  if(!info.flat && info.width){const int wd=(int)info.width;t.x%=wd;if(t.x<0)t.x+=wd;}
  const OriginalMapTile* mt=map_tile_at(tiles,t.x,t.y);if(!mt)continue;
  t.present=true;t.explored=own || map_tile_explored(*mt,viewer);
  const uint32_t items=map_tile_items(*mt,own?-1:viewer);
  t.ocean=map_tile_ocean(*mt);t.rocky=!t.ocean && (mt->val3&0xc0u)>=0x80;t.fungus=(items&map_bit::fungus)!=0;
  t.worked=((uint32_t)b->worked_tiles>>i&1u)!=0;if(t.worked)++m.worked_count;
 }
 if(forecast && forecast->valid){memcpy(m.nutrient,forecast->nutrient,sizeof(m.nutrient));memcpy(m.mineral,forecast->mineral,sizeof(m.mineral));memcpy(m.energy,forecast->energy,sizeof(m.energy));m.fresh=true;}
 else{
  m.nutrient[0]=b->nutrient_intake_2;m.nutrient[1]=b->nutrient_consumption;m.nutrient[2]=b->nutrient_surplus;
  m.mineral[0]=b->mineral_intake_2;m.mineral[1]=b->mineral_consumption;m.mineral[2]=b->mineral_surplus;
  m.energy[0]=b->energy_intake_2;m.energy[1]=b->energy_inefficiency;m.energy[2]=b->energy_surplus;
 }
 m.stored_nutrients=b->nutrients_accumulated;m.stored_minerals=b->minerals_accumulated;
 m.economy=b->economy_total;m.labs=b->labs_total;m.psych=b->psych_total;
 // queue_items[0] is current production; queue_size counts the items after it.
 const int32_t queued=b->queue_size<0?0:(b->queue_size>9?9:b->queue_size);
 for(int32_t q=0;q<=queued;++q){const int32_t id=b->queue_items[q];char* s=m.queue[m.queue_count];
  if(id>=0){const OriginalPrototype* p=id<512?w.prototype((size_t)id):nullptr;
   if(p && p->name[0])copy_text(s,40,p->name);else snprintf(s,40,"Unit design %ld",(long)id);}
  else if(defs && -id<(int32_t)defs->slot_count && defs->slots[-id].name[0])copy_text(s,40,defs->slots[-id].name);
  else snprintf(s,40,"Facility %ld",(long)-id);
  ++m.queue_count;
 }
 for(unsigned id=1;id<96 && m.facility_count<64;++id){if(!(b->facilities_built[id/8]>>(id%8)&1u))continue;char* s=m.facilities[m.facility_count++];
  if(defs && id<defs->slot_count && defs->slots[id].name[0])copy_text(s,40,defs->slots[id].name);else snprintf(s,40,"Facility %u",id);}
 memcpy(&out,&m,sizeof(m));return true;
}
size_t render_base_window(const BaseWindowModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 if(!ui_frame_valid(f) || r.w<360 || r.h<300)return 0;
 static const uint8_t fill[3]={7,19,22},edge[3]={70,110,96},land[3]={96,120,64},rocky[3]={128,116,100},ocean[3]={32,64,120},fungus[3]={150,70,140},
  unknown[3]={0,0,0},worked[3]={240,212,96},centre[3]={240,240,240},trough[3]={24,40,44};
 static const uint8_t bar_food[3][3]={{70,130,60},{120,210,90},{0,0,0}},bar_min[3][3]={{110,90,60},{200,160,90},{0,0,0}},bar_en[3][3]={{110,100,40},{240,220,80},{0,0,0}};
 ui_panel(f,r,fill,edge);
 size_t n=0;char line[160];
 ui_text_item(items,capacity,n,r.x+12,r.y+24,r.w-24,3,true,m.name);
 snprintf(line,sizeof(line),"Population %ld   Specialists %ld   Talents %ld   Drones %ld",(long)m.population,(long)m.specialists,(long)m.talents,(long)m.drones);
 ui_text_item(items,capacity,n,r.x+12,r.y+46,r.w-24,0,false,line);
 // Radius map: 7x7 half-width grid of small diamonds (14 px per column step).
 const int mx=r.x+96,my=r.y+132;
 for(unsigned i=0;i<21;++i){const BaseWindowTile& t=m.tiles[i];
  const int cx=mx+radius_dx[i]*13,cy=my+radius_dy[i]*13;
  const uint8_t* c=!t.present || !t.explored?unknown:t.fungus?fungus:t.ocean?ocean:t.rocky?rocky:land;
  small_diamond(f,cx,cy,13,13,c,t.worked?worked:edge);
  if(i==0)small_diamond(f,cx,cy,5,5,centre,nullptr);
  else if(t.worked)small_diamond(f,cx,cy,3,3,worked,nullptr);
 }
 snprintf(line,sizeof(line),"Worked tiles %u of 20",m.worked_count-(m.tiles[0].worked?1u:0u));
 ui_text_item(items,capacity,n,r.x+12,r.y+196,170,1,false,line);
 // Resource bars: consumption then surplus, scaled to the largest intake.
 int32_t scale=1;
 const int32_t* intakes[3]={m.nutrient,m.mineral,m.energy};
 for(const int32_t* v:intakes)if(v[0]>scale)scale=v[0];
 const int bx=r.x+196,bw=r.w-210;
 struct Row{const char* label;const int32_t* v;const uint8_t (*colours)[3];} rows[3]={{"Nutrients",m.nutrient,bar_food},{"Minerals",m.mineral,bar_min},{"Energy",m.energy,bar_en}};
 for(unsigned k=0;k<3;++k){const int y=r.y+70+k*44;
  snprintf(line,sizeof(line),"%s  %ld  -%ld  %+ld",rows[k].label,(long)rows[k].v[0],(long)rows[k].v[1],(long)rows[k].v[2]);
  ui_text_item(items,capacity,n,bx,y,bw,1,false,line);
  ui_fill(f,{bx,y+6,bw,12},trough);
  const int32_t parts[3]={rows[k].v[1],rows[k].v[2],0};ui_bar(f,{bx,y+6,bw,12},parts,rows[k].colours,scale);
 }
 snprintf(line,sizeof(line),"Stored  N %ld   M %ld   %s",(long)m.stored_nutrients,(long)m.stored_minerals,m.fresh?"(fresh forecast)":"(saved turn)");
 ui_text_item(items,capacity,n,bx,r.y+206,bw,0,false,line);
 snprintf(line,sizeof(line),"Economy %ld   Labs %ld   Psych %ld",(long)m.economy,(long)m.labs,(long)m.psych);
 ui_text_item(items,capacity,n,bx,r.y+226,bw,2,false,line);
 // Production queue (left) and facilities (right), as many lines as fit.
 const int list_top=r.y+252,line_h=18,max_lines=(r.y+r.h-8-list_top)/line_h;
 ui_text_item(items,capacity,n,r.x+12,list_top,170,1,false,"PRODUCTION");
 for(unsigned q=0;q<m.queue_count && (int)q+1<max_lines;++q){if(q)snprintf(line,sizeof(line),"%u. %s",q,m.queue[q]);else snprintf(line,sizeof(line),"> %s",m.queue[q]);
  ui_text_item(items,capacity,n,r.x+12,list_top+line_h*(int)(q+1),176,q?0:2,false,line);}
 snprintf(line,sizeof(line),"FACILITIES (%u)",m.facility_count);
 ui_text_item(items,capacity,n,bx,list_top,bw,1,false,line);
 const int columns=bw>=320?2:1,col_w=bw/columns;
 for(unsigned i=0;i<m.facility_count;++i){const int col=(int)i%columns,row=(int)i/columns+1;if(row>=max_lines)break;
  ui_text_item(items,capacity,n,bx+col*col_w,list_top+line_h*row,col_w-6,0,false,m.facilities[i]);}
 return n;
}
}
