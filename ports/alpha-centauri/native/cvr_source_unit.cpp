/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_source_unit.h"
#include "cvr_map_anchor.h"
#include "cvr_scene_blit.h"
#include "cvr_palette.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
}
CvrSourceUnit::CvrSourceUnit():cache_{},parts_(0),generation_(0),ready_(false),palette_ready_(false),rgb_{},tint_{},faction_shades_{}{}
CvrSourceUnit::~CvrSourceUnit(){reset_cache();}
void CvrSourceUnit::reset_cache(){for(auto&p:cache_){free(p);p=nullptr;}++generation_;}
bool CvrSourceUnit::overlaps_owned(const void*p,size_t n)const{if(overlap(this,sizeof(*this),p,n))return true;for(const auto&zoom:models_)for(const auto&m:zoom)if(m.overlaps_storage(p,n))return true;for(const auto*c:cache_)if(c&&overlap(c,65536,p,n))return true;return false;}
bool CvrSourceUnit::load(const void*body,size_t bn,const void*reactor,size_t rn){const CvrPartInput p[]={{body,bn},{reactor,rn}};return load_parts(p,2);}
bool CvrSourceUnit::load_speeder(const CvrPartInput*p,size_t n){return n==10?load_parts(p,10):false;}
bool CvrSourceUnit::load_parts(const CvrPartInput*p,unsigned count){
 if(!p||(count!=2&&count!=10)||overlaps_owned(p,count*sizeof(*p)))return false;
 for(unsigned i=0;i<count;++i)if(!p[i].data||!p[i].size||p[i].size>16*1024*1024||overlaps_owned(p[i].data,p[i].size))return false;
 ready_=palette_ready_=false;reset_cache();parts_=count;
 for(unsigned z=0;z<2;++z){CvrViewerUnitAnchor anchor{};if(!cvr_viewer_flat_anchor(z+1,0,0,count==2,anchor))return false;unsigned voxels=0,objects=0,frames=0,groups=0;
  for(unsigned i=0;i<count;++i){auto&m=models_[z][i];if(!m.load_at_scale(p[i].data,p[i].size,anchor.magnification)||!m.frame_count())return false;voxels+=m.voxel_count();objects+=m.object_count();frames+=m.object_count()*m.frame_count();for(unsigned j=0;j<m.object_count();++j)groups+=m.object(j)->group_count;if(voxels>(count==2?100000u:160000u)||objects>200||frames>32768||groups>8192)return false;}
 }ready_=true;return true;
}
bool CvrSourceUnit::set_game_palette(const uint8_t*rgb,size_t n,const uint8_t*tint,size_t tn){
 if(!ready_||!rgb||n!=768||(parts_==10?(!tint||tn!=3):(tint||tn))||overlaps_owned(rgb,n)||(tint&&overlaps_owned(tint,tn)))return false;
 if(palette_ready_&&!memcmp(rgb_,rgb,768)&&(parts_!=10||!memcmp(tint_,tint,3)))return true;
 uint8_t tables[2][6144];if(parts_==10)for(unsigned z=0;z<2;++z){uint8_t neutral[6144];for(unsigned s=0;s<24;++s)for(unsigned c=0;c<256;++c)neutral[s*256+c]=models_[z][4].shaded_index((uint8_t)c,s);if(!build_cvr_faction_palette(rgb,768,neutral,6144,tint,3,tables[z],6144))return false;}
 reset_cache();memcpy(rgb_,rgb,768);if(parts_==10){memcpy(tint_,tint,3);memcpy(faction_shades_,tables,sizeof(tables));}palette_ready_=true;return true;
}
void CvrSourceUnit::clear_game_palette(){if(palette_ready_){reset_cache();palette_ready_=false;}}
bool CvrSourceUnit::render_snapshot(unsigned zoom,unsigned facing,CvrRasterSurface output){
 if(!ready_||!palette_ready_||zoom<1||zoom>2||facing>7||overlaps_owned(output.pixels,65536)||overlaps_owned(output.depths,131072))return false;
 CvrViewerUnitAnchor anchor{};CvrProjectionInput input{};float light[3];if(!cvr_viewer_flat_anchor(zoom,0,0,parts_==2,anchor)||!prepare_cvr_flat_scene(facing,anchor.magnification,input,light))return false;
 CvrScenePart parts[10]{};for(unsigned i=0;i<parts_;++i){parts[i].model=&models_[zoom-1][i];if(parts_==2){parts[i].offset[0]=-400;parts[i].offset[1]=200;}else if(i==2||i==4){parts[i].shade_table=faction_shades_[zoom-1];parts[i].shade_bytes=6144;}}
 CvrSceneRenderer renderer;return renderer.render(parts,parts_,input,light,output);
}
bool CvrSourceUnit::draw(uint8_t*output,size_t capacity,unsigned width,unsigned height,int x,int y,unsigned zoom,unsigned facing){
 if(!ready_||!palette_ready_||!output||!width||!height||width>4096||height>4096||capacity<(size_t)width*height*3||overlaps_owned(output,(size_t)width*height*3)||zoom<1||zoom>2||facing>7)return false;
 CvrViewerUnitAnchor anchor{};if(!cvr_viewer_flat_anchor(zoom,x,y,parts_==2,anchor))return false;auto&cached=cache_[(zoom-1)*8+facing];
 if(!cached){auto*p=(uint8_t*)malloc(65536);auto*d=(uint16_t*)malloc(131072);if(!p||!d){free(p);free(d);return false;}const bool ok=render_snapshot(zoom,facing,{p,d,65536,256,256,256});free(d);if(!ok){free(p);return false;}cached=p;}
 const CvrSceneRect clip{0,0,(int32_t)width,(int32_t)height};return composite_cvr_scene_1x(cached,65536,rgb_,768,anchor.scene_centre.x,anchor.scene_centre.y,clip,{output,capacity,width,height,width*3});
}
}
