/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_projection.h"
#include <math.h>
#include <limits.h>
namespace alpha {
namespace {
bool finite_values(const float* p,unsigned n){for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return false;return true;}
float dot(const float* a,const float* b,unsigned stride){return (float)((double)a[0]*b[0]+(double)a[1]*b[stride]+(double)a[2]*b[2*stride]);}
void transform(const float* m,const float* v,float* out){for(unsigned r=0;r<3;++r)out[r]=dot(m+r*3,v,1);}
bool scaled_floor(double v,int32_t& out){v=floor(v);if(!isfinite(v)||v<INT32_MIN||v>INT32_MAX)return false;out=(int32_t)v;return true;}
}
bool prepare_cvr_projection(const CvrProjectionInput& in,CvrProjection& out){
 if(!finite_values(in.camera,9)||!finite_values(in.model,9)||!finite_values(in.camera_position,3)||!finite_values(in.model_position,3)||!finite_values(in.model_centre,3)||!finite_values(in.model_offset,3)||!finite_values(in.screen_offset,2))return false;
 const float params[]={in.model_scale,in.screen_scale,in.magnification,in.perspective,in.near_z,in.far_z,in.depth_min,in.depth_max};
 if(!finite_values(params,8)||in.model_scale<=0||in.screen_scale<=0||in.magnification<=0||in.far_z<=in.near_z||in.depth_max<=in.depth_min||!in.width||!in.height||in.width>4096||in.height>4096||in.clip_extent_x<=0||in.clip_extent_y<=0)return false;
 CvrProjection p{};float delta[3],centre[3];
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)p.basis[r*3+c]=(float)(dot(in.camera+r*3,in.model+c,3)*(double)in.model_scale);
 for(unsigned i=0;i<3;++i)delta[i]=(float)((double)in.model_position[i]-in.camera_position[i]);
 transform(in.camera,delta,p.origin);transform(p.basis,in.model_centre,centre);
 const float centre_z=(float)((double)centre[2]+p.origin[2]);
 p.perspective_factor=centre_z>0&&in.perspective>0?(float)(1.0/((double)centre_z*in.perspective+1.0)):1.0f;
 const float factor=(float)((double)in.magnification*in.screen_scale*p.perspective_factor);
 const float depth_ratio=(float)(((double)in.depth_max-in.depth_min)/((double)in.far_z-in.near_z));
 for(unsigned i=0;i<6;++i)p.basis[i]=(float)((double)p.basis[i]*factor);
 for(unsigned i=0;i<2;++i){const float scaled_origin=(float)((double)p.origin[i]*factor);p.origin[i]=(float)((double)in.model_offset[i]*in.screen_scale+in.screen_offset[i]+(i?in.height>>1:in.width>>1)+.5+scaled_origin);}
 p.origin[2]=(float)((double)p.origin[2]-in.near_z);
 for(unsigned i=6;i<9;++i)p.basis[i]=(float)((double)p.basis[i]*depth_ratio);
 p.origin[2]=(float)((double)p.origin[2]*depth_ratio);
 if(in.reverse_depth){for(unsigned i=6;i<9;++i)p.basis[i]=-p.basis[i];p.origin[2]=-p.origin[2];}
 p.origin[2]=(float)((double)in.model_offset[2]+(in.reverse_depth?in.depth_max:in.depth_min)+p.origin[2]);
 p.model_pixel_scale=(float)((double)in.model_scale*factor);p.model_depth_scale=(float)((double)in.model_scale*depth_ratio);
 int32_t w,h;
 if(!scaled_floor((double)in.clip_left*in.screen_scale,p.clip_left)||!scaled_floor((double)in.clip_top*in.screen_scale,p.clip_top)||!scaled_floor(((double)in.clip_extent_x+1)*in.screen_scale,w)||!scaled_floor(((double)in.clip_extent_y+1)*in.screen_scale,h)||w==INT32_MAX||h==INT32_MAX)return false;
 p.clip_width=w+1;p.clip_height=h+1;
 const int64_t right=(int64_t)p.clip_left-1+p.clip_width,bottom=(int64_t)p.clip_top-1+p.clip_height;
 if(right<INT32_MIN||right>INT32_MAX||bottom<INT32_MIN||bottom>INT32_MAX||!finite_values(p.origin,3)||!finite_values(p.basis,9)||!isfinite(p.perspective_factor)||!isfinite(p.model_pixel_scale)||!isfinite(p.model_depth_scale))return false;
 p.clip_right=(int32_t)right;p.clip_bottom=(int32_t)bottom;out=p;return true;
}
}
