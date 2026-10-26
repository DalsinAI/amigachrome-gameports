/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_ground_scene.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static void real(float v){uint32_t bits;memcpy(&bits,&v,4);for(unsigned i=0;i<4;++i)putchar((int)((bits>>(8*i))&255));}
static bool emit(const uint8_t*relative,alpha::CvrGroundProfile profile,unsigned facing,float scale){alpha::CvrProjectionInput p{};float light[3];if(!alpha::prepare_cvr_ground_scene(relative,profile,facing,scale,p,light))return false;for(float v:p.camera)real(v);for(float v:p.camera_position)real(v);for(float v:p.model)real(v);for(float v:light)real(v);real(p.magnification);return true;}
static bool invalid_controls(){
 alpha::CvrProjectionInput p;memset(&p,0xa5,sizeof(p));unsigned char before[sizeof(p)];memcpy(before,&p,sizeof(p));float light[3]={17,18,19};uint8_t codes[4]={4,4,4,5};
 const auto unchanged=[&](){return !memcmp(&p,before,sizeof(p))&&light[0]==17&&light[1]==18&&light[2]==19;};
 const float badscales[]={0,-1,NAN,INFINITY,1025};for(float scale:badscales)if(alpha::prepare_cvr_ground_scene(codes,alpha::CvrGroundProfile::basicRover,0,scale,p,light)||!unchanged())return false;
 if(alpha::prepare_cvr_ground_scene(codes,(alpha::CvrGroundProfile)99,0,.07f,p,light)||!unchanged())return false;
 if(alpha::prepare_cvr_ground_scene(codes,alpha::CvrGroundProfile::basicRover,8,.07f,p,light)||!unchanged())return false;
 if(alpha::prepare_cvr_ground_scene(nullptr,alpha::CvrGroundProfile::basicRover,0,.07f,p,light)||!unchanged())return false;
 if(alpha::prepare_cvr_ground_scene(codes,alpha::CvrGroundProfile::basicRover,0,.07f,p,nullptr)||!unchanged())return false;
 for(unsigned i=0;i<4;++i){const uint8_t saved=codes[i];codes[i]=0;if(alpha::prepare_cvr_ground_scene(codes,alpha::CvrGroundProfile::basicRover,0,.07f,p,light)||!unchanged())return false;codes[i]=saved;}
 return true;
}
int main(int argc,char**argv){if(!invalid_controls())return 20;
 fprintf(stderr,"GROUND FACTORY13 invalid transactional controls PASS\n");
 if(argc==1){for(unsigned a=1;a<=7;++a)for(unsigned b=1;b<=7;++b)for(unsigned c=1;c<=7;++c)for(unsigned d=1;d<=7;++d)for(unsigned f=0;f<8;++f){const uint8_t relative[]={(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};if(!emit(relative,alpha::CvrGroundProfile::basicRover,f,.07f))return 20;}}
 else{if(argc!=6)return 20;uint8_t relative[4];for(unsigned i=0;i<4;++i)relative[i]=(uint8_t)atoi(argv[1+i]);const int zoom=atoi(argv[5]);for(unsigned recipe=0;recipe<2;++recipe)for(unsigned facing=0;facing<8;++facing){const float source_scale=(float)((double).07f*(zoom+16)/16);const float scale=recipe?source_scale:(float)((double)source_scale*1.25);if(!emit(relative,recipe?alpha::CvrGroundProfile::basicRover:alpha::CvrGroundProfile::basicInfantry,facing,scale))return 20;}}
 return ferror(stdout)?20:0;
}
