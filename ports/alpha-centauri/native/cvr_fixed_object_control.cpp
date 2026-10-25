/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_fixed_transform.h"
#include "cvr_lighting.h"
#include "cvr_raster.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
static int run_case(float scale,const int16_t seed[3],bool bytes){const float matrix[]={scale,0,0,0,scale,0,0,0,scale},origin[]={1,1,100};const int16_t position[2][3]={{(int16_t)(seed[0]+1),seed[1],(int16_t)(seed[2]+1)},{(int16_t)(seed[0]+1),(int16_t)(seed[1]+1),(int16_t)(seed[2]+2)}};alpha::CvrFixedBasis b{};alpha::CvrFixedSeed s{};if(!alpha::prepare_cvr_fixed_basis(matrix,b)||!alpha::prepare_cvr_fixed_seed(origin,matrix,seed,s))return 20;
 static uint8_t lattice[alpha::cvr_shade_lattice_size],shades[2048],palette[6144],pixels[64];memset(pixels,0,sizeof(pixels));uint16_t depths[64];for(auto&d:depths)d=65535;for(unsigned sh=0;sh<24;++sh)for(unsigned c=0;c<256;++c)palette[sh*256+c]=(uint8_t)(c+sh*13);
 if(!alpha::build_cvr_shade_lattice(lattice,sizeof(lattice))||!alpha::cvr_shades_for_light(lattice,sizeof(lattice),.2f*scale,-.5f*scale,.84f*scale,shades,sizeof(shades)))return 20;
 alpha::CvrRasterPoint points[2]{};for(unsigned i=0;i<2;++i){alpha::cvr_fixed_position(b,s,position[i],points[i].x,points[i].y,points[i].z);points[i].normal=0;points[i].color=i?9:7;}
 const alpha::CvrRasterClip clip{0,0,3,3};alpha::CvrRasterSurface surface{pixels,depths,64,8,8,8};if(!alpha::raster_cvr_points(points,2,shades,sizeof(shades),palette,sizeof(palette),clip,false,surface))return 20;
 if(scale==1&&seed[0]==0&&(pixels[10]!=202||pixels[18]!=204||depths[10]!=101||depths[18]!=102))return 20;
 if(bytes){fwrite(pixels,1,64,stdout);for(auto d:depths){putchar(d&255);putchar(d>>8);}return ferror(stdout)?20:0;}
 alpha::CvrFixedBasis previous=b;float invalid[9];memcpy(invalid,matrix,sizeof(matrix));invalid[0]=NAN;if(alpha::prepare_cvr_fixed_basis(invalid,b)||memcmp(&previous,&b,sizeof(b)))return 20;
 auto old=s;const float bad_origin[]={INFINITY,0,0};if(alpha::prepare_cvr_fixed_seed(bad_origin,matrix,seed,s)||memcmp(&old,&s,sizeof(s)))return 20;
 return 0;}
int main(int argc,char**){const int16_t seeds[2][3]={{0,0,0},{1,-1,1}};const float scales[]={1,.5f,.125f,1.001f};for(float scale:scales)for(const auto&seed:seeds)if(run_case(scale,seed,argc>1))return 20;if(argc==1)puts("CVR FIXED OBJECT 8normal/no-shadow cases+16invalid transactional controls PASS");return 0;}
