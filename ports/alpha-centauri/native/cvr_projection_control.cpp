/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_projection.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
static alpha::CvrProjectionInput valid(){alpha::CvrProjectionInput i{};for(unsigned a=0;a<3;++a)i.camera[a*3+a]=i.model[a*3+a]=1;i.model_scale=i.screen_scale=i.magnification=1;i.far_z=i.depth_max=65535;i.width=i.height=256;i.clip_extent_x=i.clip_extent_y=255;return i;}
int main(){auto i=valid();alpha::CvrProjection out{};if(!alpha::prepare_cvr_projection(i,out)||out.origin[0]!=128.5f||out.origin[1]!=128.5f||out.origin[2]!=0||out.clip_width!=257||out.clip_right!=256)return 20;
 unsigned checks=1;
 for(unsigned k=0;k<25;++k){i=valid();switch(k){case 0:i.camera[3]=NAN;break;case 1:i.model[4]=INFINITY;break;case 2:i.camera_position[0]=NAN;break;case 3:i.model_position[1]=NAN;break;case 4:i.model_centre[2]=NAN;break;case 5:i.model_offset[2]=INFINITY;break;case 6:i.screen_offset[0]=NAN;break;case 7:i.model_scale=0;break;case 8:i.screen_scale=-1;break;case 9:i.magnification=0;break;case 10:i.perspective=NAN;break;case 11:i.near_z=i.far_z;break;case 12:i.depth_max=i.depth_min;break;case 13:i.width=0;break;case 14:i.height=4097;break;case 15:i.clip_extent_x=-1;break;case 16:i.clip_extent_y=-1;break;case 17:i.screen_scale=FLT_MAX;break;case 18:i.model_scale=FLT_MAX;i.model[0]=FLT_MAX;break;case 19:i.clip_extent_x=INT32_MAX;break;case 20:i.near_z=NAN;break;case 21:i.depth_min=-INFINITY;break;case 22:i.model_position[0]=FLT_MAX;i.camera_position[0]=-FLT_MAX;break;case 23:i.camera[0]=FLT_MAX;i.model[0]=FLT_MAX;break;case 24:i.clip_left=INT32_MIN;i.clip_top=INT32_MIN;i.screen_scale=2;break;}
  memset(&out,0x5a,sizeof(out));alpha::CvrProjection before=out;if(alpha::prepare_cvr_projection(i,out)||memcmp(&out,&before,sizeof(out)))return 20;++checks;
 }
 printf("CVR PROJECTION %u valid/invalid transactional controls PASS\n",checks);return 0;
}
