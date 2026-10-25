/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_fixed_transform.h"
#include <math.h>
#include <limits.h>
#include <string.h>
namespace alpha {
namespace {
bool rounded(float v,int32_t& out){const float scaled=v*256.0f;if(!isfinite(scaled))return false;const double n=trunc((double)scaled+(scaled<0?-.5:.5));if(n<INT32_MIN||n>INT32_MAX)return false;out=(int32_t)n;return true;}
int16_t narrow(int32_t v){const uint16_t u=(uint16_t)(uint32_t)v;int16_t result;memcpy(&result,&u,2);return result;}
int32_t signed_word(uint32_t v){int32_t r;memcpy(&r,&v,4);return r;}
}
bool prepare_cvr_fixed_basis(const float* basis,CvrFixedBasis& out){if(!basis)return false;CvrFixedBasis p{};for(unsigned i=0;i<9;++i){int32_t n;if(!rounded(basis[i],n))return false;if(i<6)p.xy[i]=narrow(n);else p.z[i-6]=n;}out=p;return true;}
bool prepare_cvr_fixed_seed(const float* origin,const float* basis,const int16_t* seed,CvrFixedSeed& out){if(!origin||!basis||!seed)return false;CvrFixedSeed p{};int32_t n[3];for(unsigned r=0;r<3;++r){if(!isfinite(origin[r]))return false;for(unsigned j=0;j<3;++j)if(!isfinite(basis[r*3+j]))return false;const float transformed=(float)((double)basis[r*3]*seed[0]+(double)basis[r*3+1]*seed[1]+(double)basis[r*3+2]*seed[2]);const float value=(float)((double)origin[r]+transformed);if(!rounded(value,n[r]))return false;p.local[r]=seed[r];}p.x=n[0];p.y=n[1];p.z=(uint32_t)n[2]<<8;out=p;return true;}
void cvr_fixed_position(const CvrFixedBasis& b,const CvrFixedSeed& s,const int16_t* position,int32_t& x,int32_t& y,uint32_t& z){uint32_t xx=(uint32_t)s.x,yy=(uint32_t)s.y,zz=s.z;for(unsigned i=0;i<3;++i){const int32_t d=(int32_t)position[i]-s.local[i];xx+=(uint32_t)((int64_t)b.xy[i]*d);yy+=(uint32_t)((int64_t)b.xy[3+i]*d);zz+=(uint32_t)((int64_t)b.z[i]*d)<<8;}x=signed_word(xx);y=signed_word(yy);z=zz;}
}
