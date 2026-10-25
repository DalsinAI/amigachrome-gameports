/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_source_unit.h"
#include "cvr_map_anchor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::CvrSourceUnit unit;static uint8_t pixels[65536],palette[768],tint[3],canvas[928*480*3];static uint16_t depths[65536];
static bool file(const char*path,void*&p,size_t&n){FILE*f=fopen(path,"rb");if(!f)return false;if(fseek(f,0,2)){fclose(f);return false;}const long size=ftell(f);rewind(f);if(size<=0||size>16*1024*1024){fclose(f);return false;}p=malloc(size);n=(size_t)size;const bool ok=p&&fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);p=nullptr;}return ok;}
static bool drawing(unsigned zoom,unsigned facing,const uint8_t*configured_tint=tint){
 const int positions[][2]={{464,240},{0,0},{927,479},{-20,240},{940,240}};
 for(const auto&position:positions){memset(canvas,83,sizeof(canvas));if(!unit.draw(canvas,sizeof(canvas),928,480,position[0],position[1],zoom,facing))return false;alpha::CvrViewerUnitAnchor anchor{};if(!alpha::cvr_viewer_flat_anchor(zoom,position[0],position[1],false,anchor))return false;for(int y=0;y<480;++y)for(int x=0;x<928;++x){const int sx=x-anchor.scene_centre.x+128,sy=y-anchor.scene_centre.y+128;unsigned index=9;if(sx>=0&&sy>=0&&sx<256&&sy<256)index=pixels[sy*256+sx];for(unsigned c=0;c<3;++c)if(canvas[((size_t)y*928+x)*3+c]!=(index==9?83:palette[index*3+c]))return false;}}
 const unsigned generation=unit.cache_generation();if(!unit.set_game_palette(palette,768,configured_tint,3))return false;return unit.cache_generation()==generation;
}
int main(int argc,char**argv){if(argc!=5)return 20;void*p=nullptr;size_t n=0;if(!file(argv[3],p,n)||n!=768){free(p);return 20;}memcpy(palette,p,768);free(p);if(!file(argv[4],p,n)||n!=3){free(p);return 20;}memcpy(tint,p,3);free(p);
 const char*rover[]={"vrcp00.cvr","vgmc.cvr","vgmcp.cvr","vb.cvr","vbp.cvr","vsp.cvr","vsptb.cvr","vsptf.cvr","vlights.cvr","vr00.cvr"};const char*infantry[]={"VI.cvr","Viptr00.cvr"};unsigned draws=0;
 for(unsigned recipe=0;recipe<2;++recipe){alpha::CvrPartInput parts[10]{};void*owned[10]{};const unsigned count=recipe?10:2;for(unsigned i=0;i<count;++i){char path[1024];if(snprintf(path,sizeof(path),"%s/%s",recipe?argv[1]:argv[2],recipe?rover[i]:infantry[i])>=(int)sizeof(path)||!file(path,owned[i],parts[i].size))return 20;parts[i].data=owned[i];}bool loaded=recipe?unit.load_speeder(parts,count):unit.load(parts[0].data,parts[0].size,parts[1].data,parts[1].size);for(unsigned i=0;i<count;++i){memset(owned[i],0xa5,parts[i].size);free(owned[i]);}if(!loaded)return 20;
 if(unit.render_snapshot(1,0,{pixels,depths,65536,256,256,256}))return 20;
 if(!unit.set_game_palette(palette,768,recipe?tint:nullptr,recipe?3:0))return 20;
 const unsigned initial_generation=unit.cache_generation();auto alias_surface=alpha::CvrRasterSurface{(uint8_t*)&unit,depths,65536,256,256,256};if(unit.render_snapshot(1,0,alias_surface)||unit.draw((uint8_t*)&unit+sizeof(unit)-1,sizeof(canvas),928,480,0,0,1,0)||unit.set_game_palette((const uint8_t*)&unit,768,recipe?tint:nullptr,recipe?3:0)||unit.cache_generation()!=initial_generation)return 20;
 for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){if(!unit.render_snapshot(zoom,facing,{pixels,depths,65536,256,256,256}))return 20;fwrite(pixels,1,65536,stdout);for(auto z:depths){putchar(z&255);putchar(z>>8);}if(recipe){if(!drawing(zoom,facing))return 20;draws+=5;}}
 if(recipe){static uint8_t original_pixels[65536];memcpy(original_pixels,pixels,65536);uint8_t alternate[3];for(unsigned c=0;c<3;++c)alternate[c]=(uint8_t)(255-tint[c]);const unsigned old=unit.cache_generation();if(!unit.set_game_palette(palette,768,alternate,3)||unit.cache_generation()==old||!unit.render_snapshot(2,7,{pixels,depths,65536,256,256,256})||!memcmp(original_pixels,pixels,65536)||!drawing(2,7,alternate))return 20;draws+=5;const unsigned changed=unit.cache_generation();if(!unit.set_game_palette(palette,768,tint,3)||unit.cache_generation()==changed||!unit.render_snapshot(2,7,{pixels,depths,65536,256,256,256})||memcmp(original_pixels,pixels,65536)||!drawing(2,7))return 20;draws+=5;}
 const unsigned before=unit.cache_generation();unit.clear_game_palette();if(unit.cache_generation()==before||unit.render_snapshot(1,0,{pixels,depths,65536,256,256,256}))return 20;
 }fprintf(stderr,"SOURCE UNIT32 indexed/depth scenes +%u independent RGB/cached/border composites PASS\n",draws);return ferror(stdout)?20:0;}
