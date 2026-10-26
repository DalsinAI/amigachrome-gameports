/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_scene.h"
#include "cvr_ground_scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
static alpha::CvrModel models[10];static alpha::CvrSceneRenderer renderer;static uint8_t pixels[65536],faction[6144];static uint16_t depths[65536];
static bool read_file(const char*path,void*&data,size_t&size){FILE*f=fopen(path,"rb");if(!f)return false;fseek(f,0,SEEK_END);const long n=ftell(f);rewind(f);if(n<=0||n>16*1024*1024){fclose(f);return false;}data=malloc(n);size=(size_t)n;const bool ok=data&&fread(data,1,size,f)==size;fclose(f);if(!ok){free(data);data=nullptr;}return ok;}
static uint32_t hash(){uint32_t h=2166136261u;for(auto v:pixels)h=(h^v)*16777619u;for(auto v:depths){h=(h^(v&255))*16777619u;h=(h^(v>>8))*16777619u;}return h;}
static unsigned alias_rejections=0;
static bool factory_alias_controls(){const uint8_t relative[4]={4,4,4,5};
 alignas(alpha::CvrProjectionInput) uint8_t arena[sizeof(alpha::CvrProjectionInput)+64];memset(arena,0xa5,sizeof(arena));auto*in=new(arena+16)alpha::CvrProjectionInput{};uint8_t before[sizeof(arena)];memcpy(before,arena,sizeof(arena));
 const ptrdiff_t offsets[]={-8,0,4,(ptrdiff_t)sizeof(*in)-4};
 for(auto offset:offsets){if(alpha::prepare_cvr_ground_scene(relative,alpha::CvrGroundProfile::basicRover,0,.07f,*in,(float*)((uint8_t*)in+offset))||memcmp(before,arena,sizeof(arena)))return false;
 ++alias_rejections;}
 if(alpha::prepare_cvr_ground_scene(relative,alpha::CvrGroundProfile::basicRover,0,.07f,*in,(float*)(UINTPTR_MAX-3))||memcmp(before,arena,sizeof(arena)))return false;
 ++alias_rejections;
 if(!alpha::prepare_cvr_ground_scene(relative,alpha::CvrGroundProfile::basicRover,0,.07f,*in,(float*)((uint8_t*)in-12)))return false;
 return alpha::prepare_cvr_ground_scene(relative,alpha::CvrGroundProfile::basicRover,0,.07f,*in,(float*)((uint8_t*)in+sizeof(*in)));
}
static bool scene_alias_controls(alpha::CvrScenePart*parts,unsigned count,const alpha::CvrProjectionInput&in,const float*light,alpha::CvrRasterSurface output){
 const uint32_t before=hash();uint8_t saved_in[sizeof(in)],saved_parts[sizeof(alpha::CvrScenePart)*10],saved_renderer[sizeof(renderer)];memcpy(saved_in,&in,sizeof(in));memcpy(saved_parts,parts,sizeof(*parts)*count);memcpy(saved_renderer,&renderer,sizeof(renderer));
 const auto reject=[&](const void*p,size_t n){if(!n)return true;struct Snapshot{void*data;~Snapshot(){free(data);}}saved{malloc(n)};if(!saved.data)return false;memcpy(saved.data,p,n);const uintptr_t address=(uintptr_t)p;const uintptr_t candidates[]={address,address+n-1,address-65535,address,address+n-1,address-131071};
  for(unsigned k=0;k<6;++k){auto bad=output;if(k>=3)bad.depths=(uint16_t*)candidates[k];else bad.pixels=(uint8_t*)candidates[k];if(renderer.render(parts,count,in,light,bad)||hash()!=before||memcmp(saved_in,&in,sizeof(in))||memcmp(saved_parts,parts,sizeof(*parts)*count)||memcmp(saved_renderer,&renderer,sizeof(renderer))||memcmp(saved.data,p,n))return false;
 ++alias_rejections;}
  return true;
 };
 // Test-only object-representation read: the standard-layout renderer owns one pointer.
 static_assert(sizeof(renderer)==sizeof(void*),"update allocation-alias test for renderer layout changes");const void*lattice=nullptr;memcpy(&lattice,&renderer,sizeof(lattice));
 if(!reject(lattice,24*2137)||!reject(&renderer,sizeof(renderer))||!reject(&in,sizeof(in))||!reject(light,12)||!reject(parts,sizeof(*parts)*count))return false;
 for(unsigned k=0;k<count;++k){const auto*m=parts[k].model;if(!reject(m,sizeof(*m))||!reject(parts[k].shade_table,parts[k].shade_bytes))return false;
  for(unsigned j=0;j<m->object_count();++j){const auto*o=m->object(j);uint32_t groups=o->group_count?1:0;while(groups<o->group_count)groups*=2;
   const void*ptrs[]={o,o->voxels,o->frames,o->groups};const size_t lengths[]={sizeof(*o),(size_t)o->count*sizeof(alpha::CvrVoxel),(size_t)m->frame_count()*sizeof(alpha::CvrFrame),(size_t)groups*sizeof(alpha::CvrGroup)};
   for(unsigned r=0;r<4;++r)if(lengths[r]){const uintptr_t a=(uintptr_t)ptrs[r];if(!m->overlaps_storage(ptrs[r],lengths[r])||!m->overlaps_storage((const void*)(a+lengths[r]-1),1)||!m->overlaps_storage((const void*)(a-1),2)||!reject(ptrs[r],lengths[r]))return false;}
  }
 }
 auto bad=output;bad.pixels=(uint8_t*)(UINTPTR_MAX-1);if(renderer.render(parts,count,in,light,bad)||hash()!=before)return false;
 ++alias_rejections;
 return true;
}
int main(int argc,char**argv){if(argc!=9)return 20;if(!factory_alias_controls())return 20;void*table=nullptr;size_t bytes=0;if(!read_file(argv[3],table,bytes)||bytes!=6144){free(table);return 20;}memcpy(faction,table,6144);free(table);uint8_t relative[4];for(unsigned i=0;i<4;++i)relative[i]=(uint8_t)atoi(argv[4+i]);const int zoom=atoi(argv[8]);const char*rover[]={"vrcp00.cvr","vgmc.cvr","vgmcp.cvr","vb.cvr","vbp.cvr","vsp.cvr","vsptb.cvr","vsptf.cvr","vlights.cvr","vr00.cvr"};const char*infantry[]={"VI.cvr","Viptr00.cvr"};
 for(unsigned recipe=0;recipe<2;++recipe)for(unsigned facing=0;facing<8;++facing){alpha::CvrProjectionInput in{};float light[3];const float base=(float)((double).07f*(zoom+16)/16),scale=recipe?base:(float)((double)base*1.25);if(!alpha::prepare_cvr_ground_scene(relative,recipe?alpha::CvrGroundProfile::basicRover:alpha::CvrGroundProfile::basicInfantry,facing,scale,in,light))return 20;
  alpha::CvrScenePart parts[10]{};const unsigned count=recipe?10:2;for(unsigned k=0;k<count;++k){char path[1024];if(snprintf(path,sizeof(path),"%s/%s",recipe?argv[1]:argv[2],recipe?rover[k]:infantry[k])>=(int)sizeof(path))return 20;void*data=nullptr;size_t n=0;if(!read_file(path,data,n))return 20;const bool ok=models[k].load_at_scale(data,n,in.magnification);memset(data,0xa5,n);free(data);if(!ok)return 20;parts[k].model=&models[k];if(!recipe){parts[k].offset[0]=-400;parts[k].offset[1]=200;}else if(k==2||k==4){parts[k].shade_table=faction;parts[k].shade_bytes=6144;}}
  alpha::CvrRasterSurface output{pixels,depths,65536,256,256,256};if(!renderer.render(parts,count,in,light,output)||!scene_alias_controls(parts,count,in,light,output))return 20;fwrite(pixels,1,sizeof(pixels),stdout);for(auto d:depths){putchar(d&255);putchar(d>>8);}const uint32_t before=hash();auto invalid=in;invalid.width=0;if(renderer.render(parts,count,invalid,light,output)||hash()!=before)return 20;
  if(renderer.render(parts,0,in,light,output)||renderer.render(parts,count,in,nullptr,output)||hash()!=before)return 20;
  const auto saved=parts[0];parts[0].shade_bytes=1;if(renderer.render(parts,count,in,light,output)||hash()!=before)return 20;parts[0]=saved;parts[0].frame=parts[0].model->frame_count();if(renderer.render(parts,count,in,light,output)||hash()!=before)return 20;parts[0]=saved;auto overlap=output;overlap.depths=(uint16_t*)pixels;if(renderer.render(parts,count,in,light,overlap)||hash()!=before)return 20;
 }const bool ok=!ferror(stdout);fprintf(stderr,"SCENE alias rejections=%u adjacent-factory-outputs=2 PASS\n",alias_rejections);return ok?0:20;}
