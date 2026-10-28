/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic-fixture control for map overlays, minimap, message log and base
 * window. Prints stats, probe pixels and frame digests for the Python oracle.
 */
#include "map_overlays.h"
#include "ui_minimap.h"
#include "ui_messages.h"
#include "ui_basewin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
namespace {
uint8_t* read(const char* path,size_t& size){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0 || n>64*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc(n);if(!b || fread(b,1,n,f)!=(size_t)n){free(b);fclose(f);return nullptr;}fclose(f);size=(size_t)n;return b;}
uint32_t fnv(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
uint32_t le32(const uint8_t* p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
struct Map{alpha::OriginalMapTile* tiles=nullptr;alpha::MapTiles view{};~Map(){free(tiles);}};
/* Fixture: width u32, height u32, flat u8, then per tile climate,val3,owner,
 * visibility (u8 each), items u32, visible_items 7*u32. */
bool load_map(const char* path,Map& m){
 size_t n=0;uint8_t* b=read(path,n);if(!b || n<9){free(b);return false;}
 const uint32_t w=le32(b),h=le32(b+4);const size_t count=(size_t)w*h/2;
 if(!w || !h || w>4096 || h>4096 || n!=9+count*36){free(b);return false;}
 m.tiles=(alpha::OriginalMapTile*)calloc(count,sizeof(alpha::OriginalMapTile));if(!m.tiles){free(b);return false;}
 for(size_t i=0;i<count;++i){const uint8_t* p=b+9+i*36;auto& t=m.tiles[i];t.climate=p[0];t.val3=p[1];t.owner=(int8_t)p[2];t.visibility=p[3];t.items=le32(p+4);for(int k=0;k<7;++k)t.visible_items[k]=le32(p+8+4*k);}
 m.view={w,h,b[8]!=0,m.tiles,count};free(b);return true;
}
bool load_pcx(const char* path,alpha::PcxImage& image){if(!strcmp(path,"-"))return false;size_t n=0;uint8_t* b=read(path,n);const bool ok=b && image.load(b,n);free(b);return ok;}
const uint8_t factions[8][3]={{0,0,0},{230,60,60},{60,200,90},{70,110,240},{230,210,60},{200,90,220},{60,210,220},{240,150,60}};
void write_ppm(const char* path,const uint8_t* rgb,unsigned w,unsigned h){if(!path || !strcmp(path,"-"))return;FILE*f=fopen(path,"wb");if(!f)return;fprintf(f,"P6\n%u %u\n255\n",w,h);fwrite(rgb,1,(size_t)w*h*3,f);fclose(f);}
void print_items(const alpha::UiTextItem* items,size_t n){for(size_t i=0;i<n;++i)printf("TEXT %d %d %d %u %d |%s|\n",items[i].x,items[i].baseline,items[i].width,(unsigned)items[i].ink,(int)items[i].heading,items[i].text);}
}
int main(int argc,char** argv){
 if(argc<2)return 20;
 const char* mode=argv[1];
 if(!strcmp(mode,"overlays") && argc>=12){
  // MAP TEXTURE TER1 VIEWER LAYERS ZOOM CAMX CAMY W H [PPM] [probe x,y ...]
  Map m;if(!load_map(argv[2],m))return 20;alpha::PcxImage texture,ter1;const bool has_t=load_pcx(argv[3],texture),has_1=load_pcx(argv[4],ter1);
  alpha::MapOverlayStyle style{};style.texture=has_t?&texture:nullptr;style.ter1=has_1?&ter1:nullptr;memcpy(style.borders,factions,sizeof(factions));style.border_mask=0xfe;
  style.viewer=atoi(argv[5]);style.layers=(unsigned)atoi(argv[6]);
  const alpha::MapCamera cam{atoi(argv[8]),atoi(argv[9]),atoi(argv[7]),(uint32_t)atoi(argv[10]),(uint32_t)atoi(argv[11])};
  const size_t bytes=(size_t)cam.width*cam.height*3;uint8_t* rgb=(uint8_t*)malloc(bytes?bytes:1);if(!rgb)return 20;memset(rgb,19,bytes);
  // Rejections: bad zoom, short capacity, short tile array leave the frame unchanged.
  alpha::MapCamera bad=cam;bad.zoom=3;alpha::MapTiles shortm=m.view;shortm.count=shortm.count?shortm.count-1:0;
  if(alpha::draw_map_overlays(rgb,bytes,m.view,bad,style) || alpha::draw_map_overlays(rgb,bytes?bytes-1:0,m.view,cam,style) || alpha::draw_map_overlays(rgb,bytes,shortm,cam,style)){free(rgb);return 20;}
  for(size_t i=0;i<bytes;++i)if(rgb[i]!=19){free(rgb);return 20;}
  alpha::MapOverlayStats s{};if(!alpha::draw_map_overlays(rgb,bytes,m.view,cam,style,&s)){free(rgb);puts("OVERLAYS rejected");return 0;}
  printf("STATS tiles=%u unexplored=%u forest=%u fungus=%u farms=%u rivers=%u river_links=%u roads=%u road_links=%u tubes=%u tube_links=%u mines=%u solar=%u sensors=%u other=%u borders=%u art=%u glyphs=%u\n",
   s.tiles,s.unexplored,s.forest,s.fungus,s.farms,s.rivers,s.river_links,s.roads,s.road_links,s.tubes,s.tube_links,s.mines,s.solar,s.sensors,s.other_improvements,s.border_edges,s.art_cells,s.glyphs);
  printf("DIGEST %08x\n",fnv(rgb,bytes));
  for(int a=13;a<argc;++a){int x,y;if(sscanf(argv[a],"%d,%d",&x,&y)!=2 || x<0 || y<0 || x>=(int)cam.width || y>=(int)cam.height){free(rgb);return 20;}const uint8_t* p=rgb+((size_t)y*cam.width+x)*3;printf("PROBE %d,%d %u,%u,%u\n",x,y,p[0],p[1],p[2]);}
  if(argc>=13)write_ppm(argv[12],rgb,cam.width,cam.height);
  free(rgb);return 0;
 }
 if(!strcmp(mode,"minimap") && argc>=9){
  // MAP VIEWER RW RH CAMX CAMY MARKERS(x:y:f:b,...|-) [probe x,y ...]
  Map m;if(!load_map(argv[2],m))return 20;
  alpha::MinimapStyle s{};s.viewer=atoi(argv[3]);
  const uint8_t land[3]={96,120,64},rocky[3]={128,116,100},shelf[3]={48,96,150},ocean[3]={24,48,110},fungus[3]={150,70,140},black[3]={0,0,0},view[3]={250,250,250};
  memcpy(s.land,land,3);memcpy(s.rocky,rocky,3);memcpy(s.shelf,shelf,3);memcpy(s.ocean,ocean,3);memcpy(s.fungus,fungus,3);memcpy(s.unexplored,black,3);memcpy(s.view,view,3);
  memcpy(s.factions,factions,sizeof(factions));s.faction_mask=0xfe;
  const int rw=atoi(argv[4]),rh=atoi(argv[5]);const unsigned fw=(unsigned)rw+20,fh=(unsigned)rh+20;
  alpha::MinimapMarker markers[64];size_t count=0;
  if(strcmp(argv[8],"-")){char* p=argv[8];while(*p && count<64){int x,y,f,b,used=0;if(sscanf(p,"%d:%d:%d:%d%n",&x,&y,&f,&b,&used)!=4)return 20;markers[count++]={x,y,f,b!=0};p+=used;if(*p==',')++p;}}
  const size_t bytes=(size_t)fw*fh*3;uint8_t* rgb=(uint8_t*)malloc(bytes);if(!rgb)return 20;memset(rgb,19,bytes);
  const alpha::UiFrame frame{rgb,bytes,fw,fh};const alpha::UiRect rect{10,10,rw,rh};const alpha::MapCamera cam{atoi(argv[6]),atoi(argv[7]),1,320,192};
  alpha::MinimapStats st{};
  if(alpha::draw_minimap(frame,rect,m.view,s,nullptr,1,&cam)){free(rgb);return 20;}
  if(!alpha::draw_minimap(frame,rect,m.view,s,count?markers:nullptr,count,&cam,&st)){free(rgb);puts("MINIMAP rejected");return 0;}
  for(unsigned y=0;y<fh;++y)for(unsigned x=0;x<fw;++x)if((x<10 || y<10 || x>=10u+rw || y>=10u+rh)){const uint8_t* p=rgb+((size_t)y*fw+x)*3;if(p[0]!=19 || p[1]!=19 || p[2]!=19){free(rgb);puts("MINIMAP outside-rect write");return 20;}}
  printf("MINIMAP pixels=%u explored=%u owned=%u bases=%u units=%u view=%u digest=%08x\n",st.pixels,st.explored,st.owned,st.bases,st.units,st.view_pixels,fnv(rgb,bytes));
  for(int a=9;a<argc;++a){int x,y;if(sscanf(argv[a],"%d,%d",&x,&y)!=2)return 20;int mx=-1,my=-1;const bool hit=alpha::minimap_to_map(m.view,rect,x,y,mx,my);const uint8_t* p=(x>=0 && y>=0 && x<(int)fw && y<(int)fh)?rgb+((size_t)y*fw+x)*3:nullptr;
   printf("PROBE %d,%d %u,%u,%u map=%d %d,%d\n",x,y,p?p[0]:0,p?p[1]:0,p?p[2]:0,hit,mx,my);}
  free(rgb);return 0;
 }
 if(!strcmp(mode,"messages") && argc==3){
  // SCRIPT: lines "add TURN KIND TEXT", "scroll N", "show W H", "clear".
  size_t n=0;uint8_t* script=read(argv[2],n);if(!script)return 20;
  alpha::MessageLog log;char* text=(char*)realloc(script,n+1);if(!text){free(script);return 20;}text[n]=0;
  for(char* line=strtok(text,"\n");line;line=strtok(nullptr,"\n")){
   if(!strncmp(line,"add ",4)){long turn;int kind,used=0;if(sscanf(line+4,"%ld %d %n",&turn,&kind,&used)<2){free(text);return 20;}
    const bool ok=log.add((int32_t)turn,(alpha::MessageKind)kind,line+4+used);printf("ADD %d count=%lu total=%lu\n",ok,(unsigned long)log.count(),(unsigned long)log.total_added());}
   else if(!strncmp(line,"scroll ",7)){log.scroll(atoi(line+7));printf("OFFSET %lu\n",(unsigned long)log.offset());}
   else if(!strcmp(line,"clear")){log.clear();puts("CLEAR");}
   else if(!strncmp(line,"show ",5)){int w,h;if(sscanf(line+5,"%d %d",&w,&h)!=2){free(text);return 20;}
    const unsigned fw=(unsigned)w+8,fh=(unsigned)h+8;const size_t bytes=(size_t)fw*fh*3;uint8_t* rgb=(uint8_t*)malloc(bytes);if(!rgb){free(text);return 20;}memset(rgb,19,bytes);
    alpha::UiTextItem items[40];const size_t count=alpha::layout_message_panel(log,{rgb,bytes,fw,fh},{4,4,w,h},18,items,40);
    printf("PANEL items=%lu digest=%08x\n",(unsigned long)count,fnv(rgb,bytes));print_items(items,count);free(rgb);}
   else if(!strncmp(line,"newest ",7)){const alpha::LogMessage* msg=log.newest((size_t)atoi(line+7));if(msg)printf("NEWEST T%ld k%u x%u |%s|\n",(long)msg->turn,(unsigned)msg->kind,(unsigned)msg->repeat,msg->text);else puts("NEWEST none");}
  }
  free(text);return 0;
 }
 if(!strcmp(mode,"basewin") && argc>=6){
  // SAVE BASE VIEWER FORECAST(-|n0,n1,n2,m0,m1,m2,e0,e1,e2) [PPM] [facility names...]
  size_t n=0;uint8_t* save=read(argv[2],n);if(!save)return 20;alpha::OriginalWorld world;const bool loaded=world.load(save,n,0x12345678u)==alpha::WorldLoadError::ok;free(save);if(!loaded){puts("WORLD rejected");return 20;}
  auto* defs=(alpha::FacilityDefinitions*)calloc(1,sizeof(alpha::FacilityDefinitions));if(!defs)return 20;
  for(int a=7;a<argc && defs->slot_count<alpha::facility_capacity;++a){snprintf(defs->slots[defs->slot_count+1].name,sizeof(defs->slots[0].name),"%s",argv[a]);++defs->slot_count;}
  if(defs->slot_count)++defs->slot_count;
  alpha::BaseWindowForecast forecast{};const bool has_forecast=strcmp(argv[5],"-")!=0;
  if(has_forecast){int32_t* v[9]={&forecast.nutrient[0],&forecast.nutrient[1],&forecast.nutrient[2],&forecast.mineral[0],&forecast.mineral[1],&forecast.mineral[2],&forecast.energy[0],&forecast.energy[1],&forecast.energy[2]};
   char* p=argv[5];for(auto* x:v){*x=(int32_t)strtol(p,&p,10);if(*p==',')++p;}forecast.valid=true;}
  alpha::BaseWindowModel model;memset(&model,0x5a,sizeof(model));alpha::BaseWindowModel before;memcpy(&before,&model,sizeof(model));
  if(alpha::build_base_window(world,(size_t)world.game().base_count,defs,nullptr,-1,model) || memcmp(&before,&model,sizeof(model))){free(defs);return 20;}
  const bool built=alpha::build_base_window(world,(size_t)atoi(argv[3]),defs->slot_count?defs:nullptr,has_forecast?&forecast:nullptr,atoi(argv[4]),model);
  free(defs);if(!built){puts("BASEWIN rejected");return 0;}
  printf("MODEL name=|%s| pop=%ld faction=%ld worked=%u fresh=%d n=%ld,%ld,%ld m=%ld,%ld,%ld e=%ld,%ld,%ld queue=%u facilities=%u\n",model.name,(long)model.population,(long)model.faction,model.worked_count,(int)model.fresh,
   (long)model.nutrient[0],(long)model.nutrient[1],(long)model.nutrient[2],(long)model.mineral[0],(long)model.mineral[1],(long)model.mineral[2],(long)model.energy[0],(long)model.energy[1],(long)model.energy[2],model.queue_count,model.facility_count);
  for(unsigned i=0;i<21;++i){const auto& t=model.tiles[i];printf("TILE %u %d,%d p%d w%d o%d r%d f%d e%d\n",i,t.x,t.y,t.present,t.worked,t.ocean,t.rocky,t.fungus,t.explored);}
  for(unsigned i=0;i<model.queue_count;++i)printf("QUEUE |%s|\n",model.queue[i]);
  for(unsigned i=0;i<model.facility_count;++i)printf("FACILITY |%s|\n",model.facilities[i]);
  const unsigned fw=520,fh=420;const size_t bytes=(size_t)fw*fh*3;uint8_t* rgb=(uint8_t*)malloc(bytes);if(!rgb)return 20;memset(rgb,19,bytes);
  alpha::UiTextItem items[96];
  if(alpha::render_base_window(model,{rgb,bytes,fw,fh},{10,10,359,300},items,96)){free(rgb);return 20;}
  const size_t count=alpha::render_base_window(model,{rgb,bytes,fw,fh},{10,10,500,400},items,96);
  printf("RENDER items=%lu digest=%08x\n",(unsigned long)count,fnv(rgb,bytes));print_items(items,count);
  if(argc>=7)write_ppm(argv[6],rgb,fw,fh);
  free(rgb);return 0;
 }
 return 20;
}
