/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_terrain_normal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
static void word(unsigned v){for(unsigned i=0;i<4;++i)putchar((int)((v>>(8*i))&255));}
int main(){
 for(unsigned a=1;a<=7;++a)for(unsigned b=1;b<=7;++b)for(unsigned c=1;c<=7;++c)for(unsigned d=1;d<=7;++d){
  uint8_t codes[4]={(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};float v[3];if(!alpha::cvr_terrain_normal(codes,v))return 20;
  for(unsigned i=0;i<4;++i)word(codes[i]);
  for(unsigned i=0;i<3;++i){unsigned bits;memcpy(&bits,v+i,4);word(bits);}
  float camera[9],heading;if(!alpha::cvr_terrain_camera(v,camera,&heading))return 20;
  for(unsigned i=0;i<9;++i){unsigned bits;memcpy(&bits,camera+i,4);word(bits);}
 }
 float v[3]={7,8,9};uint8_t invalid[4]={1,0,1,1};if(alpha::cvr_terrain_normal(invalid,v)||v[0]!=7||v[1]!=8||v[2]!=9)return 20;
 if(alpha::cvr_terrain_normal(nullptr,v)||alpha::cvr_terrain_normal(invalid,nullptr))return 20;
 for(unsigned k=0;k<4;++k)for(unsigned bad=0;bad<3;++bad){uint8_t codes[4]={1,1,1,1};codes[k]=(uint8_t)(bad==0?0:bad==1?8:255);if(alpha::cvr_terrain_normal(codes,v)||v[0]!=7||v[1]!=8||v[2]!=9)return 20;}
 float output[10];for(float&f:output)f=19;
 const float invalid_normals[4][3]={{0,0,0},{NAN,0,1},{0,INFINITY,1},{FLT_MAX,FLT_MAX,FLT_MAX}};
 for(const auto&n:invalid_normals){if(alpha::cvr_terrain_camera(n,output,output+9))return 20;for(float f:output)if(f!=19)return 20;}
 const float flat[3]={0,0,1};for(unsigned k=0;k<9;++k){if(alpha::cvr_terrain_camera(flat,output,output+k))return 20;for(float f:output)if(f!=19)return 20;}
 if(!alpha::cvr_terrain_camera(flat,output,output+9))return 20;
 fprintf(stderr,"TERRAIN NORMAL/CAMERA2401 exact vectors +29null/invalid/alias/adjacency controls PASS\n");
 return 0;
}
