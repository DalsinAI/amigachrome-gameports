/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_projection.h"
#include <stdio.h>
#include <string.h>
static bool u32(FILE*f,uint32_t& v){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return true;}
static bool real(FILE*f,float&v){uint32_t u;if(!u32(f,u))return false;memcpy(&v,&u,4);return true;}
static void word(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(v>>(8*i));fwrite(b,1,4,stdout);}
static void emit(float v){uint32_t u;memcpy(&u,&v,4);word(u);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;char magic[4];uint32_t count;if(fread(magic,1,4,f)!=4||memcmp(magic,"ACPJ",4)||!u32(f,count)||count>4096){fclose(f);return 20;}
 for(uint32_t k=0;k<count;++k){alpha::CvrProjectionInput i{};alpha::CvrProjection o{};
  float* arrays[]={i.camera,i.model,i.camera_position,i.model_position,i.model_centre};const unsigned lengths[]={9,9,3,3,3};
  for(unsigned a=0;a<5;++a)for(unsigned b=0;b<lengths[a];++b)if(!real(f,arrays[a][b])){fclose(f);return 20;}
  float* values[]={&i.model_scale,&i.screen_scale,&i.magnification,&i.perspective,&i.near_z,&i.far_z,&i.depth_min,&i.depth_max,&i.model_offset[0],&i.model_offset[1],&i.model_offset[2],&i.screen_offset[0],&i.screen_offset[1]};
  for(auto*v:values)if(!real(f,*v)){fclose(f);return 20;}
  uint32_t flip,clip[4];if(!u32(f,i.width)||!u32(f,i.height)||!u32(f,flip)||flip>1){fclose(f);return 20;}i.reverse_depth=flip!=0;for(auto&c:clip)if(!u32(f,c)){fclose(f);return 20;}memcpy(&i.clip_left,&clip[0],4);memcpy(&i.clip_top,&clip[1],4);memcpy(&i.clip_extent_x,&clip[2],4);memcpy(&i.clip_extent_y,&clip[3],4);
  if(!alpha::prepare_cvr_projection(i,o)){fclose(f);return 20;}
  for(float v:o.origin){emit(v);}for(float v:o.basis){emit(v);}emit(o.perspective_factor);emit(o.model_pixel_scale);emit(o.model_depth_scale);word((uint32_t)o.clip_left);word((uint32_t)o.clip_top);word((uint32_t)o.clip_right);word((uint32_t)o.clip_bottom);word((uint32_t)o.clip_width);word((uint32_t)o.clip_height);
 }
 const bool ok=fgetc(f)==EOF&&!ferror(f)&&!ferror(stdout);fclose(f);return ok?0:20;
}
