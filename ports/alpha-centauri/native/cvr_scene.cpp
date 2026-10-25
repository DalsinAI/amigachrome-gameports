/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_scene.h"
#include "cvr_fixed_transform.h"
#include "cvr_lighting.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlaps(const void*a,size_t as,const void*b,size_t bs){if(!as||!bs)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return as>UINTPTR_MAX-x||bs>UINTPTR_MAX-y||(x<y+bs&&y<x+as);}
void transform(const float*m,const float*v,float*out){for(unsigned r=0;r<3;++r)out[r]=(float)((double)m[r*3]*v[0]+(double)m[r*3+1]*v[1]+(double)m[r*3+2]*v[2]);}
void multiply(const float*a,const float*b,float*out){for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)out[r*3+c]=(float)((double)a[r*3]*b[c]+(double)a[r*3+1]*b[3+c]+(double)a[r*3+2]*b[6+c]);}
bool inverse(const float*m,float*out){const double cof[9]={(double)m[4]*m[8]-(double)m[5]*m[7],(double)m[2]*m[7]-(double)m[1]*m[8],(double)m[1]*m[5]-(double)m[2]*m[4],(double)m[5]*m[6]-(double)m[3]*m[8],(double)m[0]*m[8]-(double)m[2]*m[6],(double)m[2]*m[3]-(double)m[0]*m[5],(double)m[3]*m[7]-(double)m[4]*m[6],(double)m[1]*m[6]-(double)m[0]*m[7],(double)m[0]*m[4]-(double)m[1]*m[3]};const double det=(double)m[0]*cof[0]+(double)m[1]*cof[3]+(double)m[2]*cof[6];if(!isfinite(det)||det==0)return false;for(unsigned i=0;i<9;++i){out[i]=(float)(cof[i]/det);if(!isfinite(out[i]))return false;}return true;}
}
bool prepare_cvr_flat_scene(unsigned facing,float magnification,CvrProjectionInput& out,float* light){
 if(facing>7||!light||overlaps(&out,sizeof(out),light,3*sizeof(float))||!isfinite(magnification)||magnification<=0||magnification>1024)return false;
 CvrProjectionInput p{};const float tilt=1.0466667f,c=(float)cos((double)tilt),s=(float)sin((double)tilt),angle=(float)((double)((facing+5)&7)*.785375f),co=(float)cos((double)angle),si=(float)sin((double)angle);
 const float camera[]={1,0,0,0,c,-s,-0.0f,-s,-c},model[]={co,-si,0,si,co,0,0,0,1},view[]={-.5773500204086304f,.5773500204086304f,.5773500204086304f};memcpy(p.camera,camera,sizeof(camera));memcpy(p.model,model,sizeof(model));
 for(unsigned i=0;i<3;++i)p.camera_position[i]=(float)((double)p.camera[6+i]*-10000.0f);
 float inv[9],world[3];if(!inverse(camera,inv))return false;transform(inv,view,world);p.model_scale=p.screen_scale=1;p.magnification=magnification;p.near_z=-65535;p.far_z=p.depth_max=65535;p.width=p.height=256;p.clip_extent_x=p.clip_extent_y=256;out=p;memcpy(light,world,sizeof(world));return true;
}
CvrSceneRenderer::CvrSceneRenderer():lattice_(nullptr){}
CvrSceneRenderer::~CvrSceneRenderer(){free(lattice_);}
bool CvrSceneRenderer::render(const CvrScenePart*parts,unsigned count,const CvrProjectionInput& input,const float* world_light,CvrRasterSurface output){
 if(!parts||!count||count>10||!world_light||!output.pixels||!output.depths||output.width!=256||output.height!=256||output.stride!=256||output.capacity<65536||input.width!=256||input.height!=256)return false;
 const auto output_overlaps=[&](const void*p,size_t n){return overlaps(output.pixels,65536,p,n)||overlaps(output.depths,131072,p,n);};
 if(overlaps(output.pixels,65536,output.depths,131072)||output_overlaps(this,sizeof(*this))||output_overlaps(lattice_,lattice_?cvr_shade_lattice_size:0)||output_overlaps(parts,(size_t)count*sizeof(*parts))||output_overlaps(&input,sizeof(input))||output_overlaps(world_light,3*sizeof(float)))return false;
 for(unsigned i=0;i<3;++i)if(!isfinite(world_light[i]))return false;
 CvrProjection p{};if(!prepare_cvr_projection(input,p))return false;
 uint32_t objects=0,voxels=0;for(unsigned k=0;k<count;++k){const auto&part=parts[k];if(!part.model||part.frame>=part.model->frame_count()||(!part.shade_table&&part.shade_bytes)||(part.shade_table&&part.shade_bytes!=6144))return false;if(output_overlaps(part.shade_table,part.shade_bytes)||part.model->overlaps_storage(output.pixels,65536)||part.model->overlaps_storage(output.depths,131072))return false;for(float o:part.offset)if(!isfinite(o))return false;objects+=part.model->object_count();voxels+=part.model->voxel_count();if(objects>200||voxels>200000)return false;}
 if(!lattice_){lattice_=(uint8_t*)malloc(cvr_shade_lattice_size);if(!lattice_)return false;if(!build_cvr_shade_lattice(lattice_,cvr_shade_lattice_size)){free(lattice_);lattice_=nullptr;return false;}}
 auto*pixels=(uint8_t*)malloc(65536);auto*depths=(uint16_t*)malloc(131072);if(!pixels||!depths){free(pixels);free(depths);return false;}memset(pixels,9,65536);for(unsigned i=0;i<65536;++i)depths[i]=65535;
 CvrRasterSurface target{pixels,depths,65536,256,256,256};CvrRasterClip clip{p.clip_left>0?p.clip_left:0,p.clip_top>0?p.clip_top:0,p.clip_right<255?p.clip_right:255,p.clip_bottom<255?p.clip_bottom:255};bool ok=true;float view_light[3];transform(input.camera,world_light,view_light);
 for(unsigned k=0;ok&&k<count;++k){const auto&part=parts[k];uint8_t palette[6144];if(part.shade_table)memcpy(palette,part.shade_table,6144);else for(unsigned s=0;s<24;++s)for(unsigned c=0;c<256;++c)palette[s*256+c]=part.model->shaded_index((uint8_t)c,s);
  for(unsigned j=0;ok&&j<part.model->object_count();++j){const auto*o=part.model->object(j);const auto&f=o->frames[part.frame];if(f.flags&0x80)continue;
   float location[3],origin[3],basis[9];for(unsigned a=0;a<3;++a)location[a]=(float)((double)f.location[a]+part.offset[a]);transform(p.basis,location,origin);for(unsigned a=0;a<3;++a)origin[a]=(float)((double)origin[a]+p.origin[a]);const float margin=(float)((double)o->units*p.model_pixel_scale*128);if(origin[2]<input.depth_min||origin[2]>input.depth_max||origin[0]<-margin||origin[1]<-margin||origin[0]>input.clip_extent_x+margin||origin[1]>input.clip_extent_y+margin)continue;
   multiply(p.basis,f.matrix,basis);for(float&v:basis)v=(float)((double)v*o->units);CvrFixedBasis fixed{};float inv[9],scaled_light[3],object_light[3];if(!prepare_cvr_fixed_basis(basis,fixed)||!inverse(basis,inv)){ok=false;break;}
   const float xy=(float)((double)o->units*p.model_pixel_scale),z=(float)((double)o->units*p.model_depth_scale);scaled_light[0]=(float)((double)view_light[0]*xy);scaled_light[1]=(float)((double)view_light[1]*xy);scaled_light[2]=(float)((double)view_light[2]*z);transform(inv,scaled_light,object_light);uint8_t shades[2048];if(!cvr_shades_for_light(lattice_,cvr_shade_lattice_size,object_light[0],object_light[1],object_light[2],shades,sizeof(shades))){ok=false;break;}
   for(unsigned g=0;ok&&g<o->group_count;++g){const auto&group=o->groups[g];CvrFixedSeed seed{};if(!prepare_cvr_fixed_group_seed(origin,basis,group.seed,seed)){ok=false;break;}if(!group.count)continue;auto*points=(CvrRasterPoint*)malloc((size_t)group.count*sizeof(CvrRasterPoint));if(!points){ok=false;break;}
    for(unsigned i=0;i<group.count;++i){const auto&v=o->voxels[group.first+i];const int16_t local[]={v.z,v.x,v.y};cvr_fixed_position(fixed,seed,local,points[i].x,points[i].y,points[i].z);points[i].normal=v.normal;points[i].color=v.color;}
    ok=raster_cvr_points(points,group.count,shades,sizeof(shades),palette,sizeof(palette),clip,(float)((double)p.model_pixel_scale*o->units)>.5f,target);free(points);
   }
  }
 }
 if(ok){memcpy(output.pixels,pixels,65536);memcpy(output.depths,depths,131072);}free(pixels);free(depths);return ok;
}
}
