/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_lighting.h"
#include <math.h>
namespace alpha {
namespace { constexpr double pi=3.14159265358979323846; }
bool build_cvr_shade_lattice(uint8_t* out,size_t size) {
 if(!out || size!=cvr_shade_lattice_size)return false;
 double sine[24],cosine[24],azimuth[89];
 for(unsigned i=0;i<24;++i){sine[i]=sin(i*pi/46);cosine[i]=cos(i*pi/46);}
 for(unsigned i=0;i<89;++i)azimuth[i]=cos(i*2*pi/89);
 for(unsigned t=0;t<24;++t)for(unsigned j=0;j<2137;++j){
  unsigned polar=(j+88)/89;if(polar>23)polar=23;
  double dot=cosine[polar]*cosine[t]+sine[polar]*sine[t]*azimuth[(j+88)%89];
  dot=fabs(dot);if(dot>1)dot=1;
  int shade=int(.5+23-acos(dot)*46/pi);if(shade<0)shade=0;if(shade>23)shade=23;
  out[size_t(t)*2137+j]=uint8_t(shade);
 }
 return true;
}
bool cvr_shades_for_light(const uint8_t* lattice,size_t lattice_size,
 float x,float y,float z,uint8_t* out,size_t size,unsigned* inclination,unsigned* azimuth) {
 if(!lattice || lattice_size!=cvr_shade_lattice_size || !out || size!=2048 ||
    !isfinite(x)||!isfinite(y)||!isfinite(z))return false;
 if(z<0){x=-x;y=-y;z=-z;}
 /* Preserve the original FPATAN signed-zero axes explicitly. Some target
  * math implementations report a domain error for atan2(0,0). */
 double az;
 if(y==0)az=signbit(x)?(signbit(y)?-pi:pi):double(y);
 else if(x==0)az=signbit(y)?-pi/2:pi/2;
 else az=atan2(double(y),double(x));
 if(!isfinite(az))return false;
 if(az<0)az-=double(float(-2*pi));
 unsigned t=z>=1?0:unsigned(.5+acos(double(z))*double(float(46/pi)));
 /* The original scale divides by float-rounded 2*pi before rounding the
  * scale to float. Rounding true-double 89/(2*pi) changes the -X axis bin. */
 unsigned p=unsigned(.5+az*double(float(89/double(float(2*pi)))));if(p==89)p=0;
 if(t>=24 || p>=89)return false;
 const size_t start=size_t(t)*2137+89-p;
 for(unsigned i=0;i<2048;++i)if(lattice[start+i]>=24)return false;
 for(unsigned i=0;i<2048;++i)out[i]=lattice[start+i];
 if(inclination)*inclination=t;
 if(azimuth)*azimuth=p;
 return true;
}
}
