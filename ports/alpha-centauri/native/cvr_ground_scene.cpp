/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_ground_scene.h"
#include "cvr_terrain_normal.h"
#include <math.h>
#include <string.h>
namespace alpha {namespace {
bool overlaps(const void*a,size_t as,const void*b,size_t bs){if(!as||!bs)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return as>UINTPTR_MAX-x||bs>UINTPTR_MAX-y||(x<y+bs&&y<x+as);}
void transform(const float*m,const float*v,float*out){for(unsigned r=0;r<3;++r)out[r]=(float)((double)m[r*3]*v[0]+(double)m[r*3+1]*v[1]+(double)m[r*3+2]*v[2]);}
bool inverse(const float*m,float*out){
 // Original inverse has asymmetric precision: first minor, reciprocal and
 // seven output minors spill to float; minors3/6 remain wide until output.
 const float first=(float)((double)m[4]*m[8]-(double)m[5]*m[7]);
 const double second=(double)m[3]*m[8]-(double)m[6]*m[5],third=(double)m[3]*m[7]-(double)m[6]*m[4];
 const double det=third*m[2]+((double)m[0]*first-second*m[1]);
 if(!isfinite(det)||det==0)return false;
 const float reciprocal=(float)(1.0/det);
 const double cof[]={first,-((double)m[1]*m[8]-(double)m[2]*m[7]),(double)m[1]*m[5]-(double)m[2]*m[4],-second,(double)m[0]*m[8]-(double)m[2]*m[6],-((double)m[0]*m[5]-(double)m[2]*m[3]),third,-((double)m[0]*m[7]-(double)m[1]*m[6]),(double)m[0]*m[4]-(double)m[1]*m[3]};
 for(unsigned i=0;i<9;++i){out[i]=(float)((double)reciprocal*((i==3||i==6)?cof[i]:(float)cof[i]));if(!isfinite(out[i]))return false;}return true;
}
}
bool prepare_cvr_ground_scene(const uint8_t relative[4],CvrGroundProfile profile,unsigned facing,float magnification,CvrProjectionInput&out,float*light){
 if(!relative||!light||facing>7||overlaps(&out,sizeof(out),light,3*sizeof(float))||!isfinite(magnification)||magnification<=0||magnification>1024||(profile!=CvrGroundProfile::basicInfantry&&profile!=CvrGroundProfile::basicRover))return false;
 float normal[3],heading=0;CvrProjectionInput p{};
 if(!cvr_terrain_normal(relative,normal))return false;
 if(profile==CvrGroundProfile::basicInfantry){normal[0]=normal[1]=0;normal[2]=1;}
 if(!cvr_terrain_camera(normal,p.camera,&heading))return false;
 const float angle=(float)((double)((facing+5)&7)*.785375f-heading),co=(float)cos((double)angle),si=(float)sin((double)angle);
 const float model[]={co,si==0?0.0f:-si,(float)((double)0.0f*si+(double)co*0.0f),si,co,(float)((double)si*0.0f-(double)0.0f*co),-0.0f,0,1},view[]={-.5773500204086304f,.5773500204086304f,.5773500204086304f};memcpy(p.model,model,sizeof(model));
 for(unsigned i=0;i<3;++i)p.camera_position[i]=(float)((double)p.camera[6+i]*-10000.0f);
 float inv[9],world[3];if(!inverse(p.camera,inv))return false;transform(inv,view,world);
 p.model_scale=p.screen_scale=1;p.magnification=magnification;p.near_z=-65535;p.far_z=p.depth_max=65535;p.width=p.height=256;p.clip_extent_x=p.clip_extent_y=256;out=p;memcpy(light,world,sizeof(world));return true;
}
}
