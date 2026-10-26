/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_terrain_normal.h"
#include <math.h>
namespace alpha {namespace {
void normalize(float v[3]){
 const float inverse=(float)(1.0/sqrt((double)v[0]*v[0]+(double)v[2]*v[2]+(double)v[1]*v[1]));
 for(unsigned i=0;i<3;++i)v[i]=(float)((double)v[i]*inverse);
}
void cross(const float a[3],const float b[3],float out[3]){
 out[0]=(float)((double)a[1]*b[2]-(double)b[1]*a[2]);
 out[1]=(float)((double)b[0]*a[2]-(double)a[0]*b[2]);
 out[2]=(float)((double)a[0]*b[1]-(double)b[0]*a[1]);
 normalize(out);
}
}
bool cvr_terrain_normal(const uint8_t relative[4],float out[3]){
 if(!relative||!out)return false;
 float height[4];for(unsigned i=0;i<4;++i){if(relative[i]<1||relative[i]>7)return false;height[i]=(float)((relative[i]*30/4)-25);}
 const float west[3]={-50,0,height[0]},south[3]={0,50,height[1]},east[3]={50,0,height[2]},north[3]={0,-50,height[3]};
 float v[3],part[3];cross(south,west,v);cross(east,south,part);
 for(unsigned i=0;i<3;++i)v[i]=(float)((double)v[i]+part[i]);
 cross(north,east,part);for(unsigned i=0;i<3;++i)v[i]=(float)((double)v[i]+part[i]);
 cross(west,north,part);for(unsigned i=0;i<3;++i)v[i]=(float)((double)v[i]+part[i]);
 normalize(v);for(unsigned i=0;i<3;++i)out[i]=v[i];return true;
}
bool cvr_terrain_camera(const float normal[3],float camera[9],float* heading){
 if(!normal||!camera||!heading)return false;
 const uintptr_t ca=(uintptr_t)camera,ha=(uintptr_t)heading;
 if(ca>UINTPTR_MAX-36||ha>UINTPTR_MAX-4||(ca<ha+4&&ha<ca+36))return false;
 for(unsigned i=0;i<3;++i)if(!isfinite(normal[i]))return false;
 if(normal[0]==0&&normal[1]==0&&normal[2]==0)return false;
 const float tilt=1.0466667f,co=(float)cos((double)tilt),si=(float)sin((double)tilt);
 const float x=normal[0],y=(float)((double)co*normal[1]+(double)si*normal[2]),z=(float)(-(double)si*normal[1]+(double)co*normal[2]);
 if(!isfinite(y)||!isfinite(z))return false;
 const float angle=x==0?0.0f:(y==0?1.57f:(float)atan(((double)x/y)));
 const float length=(float)sqrt((double)x*x+(double)y*y);
 if(!isfinite(length))return false;
 const float incline=length==0?0.0f:(z==0?1.57f:(float)atan(((double)length/z)));
 const float ch=(float)cos((double)angle),sh=(float)sin((double)angle),ct=(float)cos((double)incline),st=(float)sin((double)incline);
 const float rz[9]={ch,-sh,0,sh,ch,0,0,0,1},rx[9]={1,0,0,0,ct,-st,0,st,ct};float result[9];
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)result[r*3+c]=(float)((double)rz[r*3]*rx[c]+(double)rz[r*3+2]*rx[6+c]+(double)rz[r*3+1]*rx[3+c]);
 for(unsigned i=6;i<9;++i)result[i]=-result[i];
 for(float v:result)if(!isfinite(v))return false;
 for(unsigned i=0;i<9;++i)camera[i]=result[i];
 *heading=angle;return true;
}

}
