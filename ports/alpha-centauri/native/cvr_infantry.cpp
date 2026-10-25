/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently written native assembly profile; no decompiled source included.
 */
#include "cvr_infantry.h"
#include "cvr_lighting.h"
#include "cvr_raster.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
namespace alpha { namespace {
void blit(const uint8_t* rgba,unsigned sw,unsigned sh,uint8_t* output,unsigned width,unsigned height,int cx,int foot,unsigned zoom){
 for(unsigned y=0;y<sh;++y)for(unsigned x=0;x<sw;++x){const size_t n=(size_t)y*sw+x;const int dx=cx-(int)sw/2+(int)x,dy=foot-(int)sh+8*(int)zoom+(int)y;if(rgba[n*4+3] && dx>=0 && dy>=0 && dx<(int)width && dy<(int)height)memcpy(output+((size_t)dy*width+dx)*3,rgba+n*4,3);}
}
struct Projected { float x,y,z;uint16_t normal;uint8_t color; };
bool inverse_light(const float* m,const float* light,float* out){
 const double det=(double)m[0]*(m[4]*m[8]-m[5]*m[7])-(double)m[1]*(m[3]*m[8]-m[5]*m[6])+(double)m[2]*(m[3]*m[7]-m[4]*m[6]);
 if(!isfinite(det) || fabs(det)<1e-8)return false;
 const double inv[9]={m[4]*m[8]-m[5]*m[7],m[2]*m[7]-m[1]*m[8],m[1]*m[5]-m[2]*m[4],m[5]*m[6]-m[3]*m[8],m[0]*m[8]-m[2]*m[6],m[2]*m[3]-m[0]*m[5],m[3]*m[7]-m[4]*m[6],m[1]*m[6]-m[0]*m[7],m[0]*m[4]-m[1]*m[3]};
 for(unsigned i=0;i<3;++i){out[i]=(float)((inv[i*3]*light[0]+inv[i*3+1]*light[1]+inv[i*3+2]*light[2])/det);if(!isfinite(out[i]))return false;}
 return true;
}
}
CvrInfantry::CvrInfantry():lattice_(nullptr),ready_(false),cache_{}{}
CvrInfantry::~CvrInfantry(){for(auto&c:cache_)free(c.rgba);free(lattice_);}
bool CvrInfantry::load(const void* body,size_t body_size,const void* reactor,size_t reactor_size){
 ready_=false;for(auto&c:cache_){free(c.rgba);c.rgba=nullptr;}
 if(!body_.load(body,body_size) || !reactor_.load(reactor,reactor_size) || body_.frame_count()<15 || reactor_.frame_count()<1 || (size_t)body_.voxel_count()+reactor_.voxel_count()>100000)return false;
 if(!lattice_)lattice_=(uint8_t*)malloc(cvr_shade_lattice_size);
 ready_=lattice_ && build_cvr_shade_lattice(lattice_,cvr_shade_lattice_size);return ready_;
}
bool CvrInfantry::draw(uint8_t* output,size_t capacity,unsigned width,unsigned height,int cx,int foot,unsigned zoom,unsigned facing){
 if(!ready_ || !output || !width || !height || width>4096 || height>4096 || capacity<(size_t)width*height*3 || zoom<1 || zoom>2 || facing>7 || cx<0 || foot<0 || cx>=(int)width || foot>=(int)height)return false;
 const unsigned sw=128*zoom,sh=144*zoom;const size_t cells=(size_t)sw*sh;
 Cache& cached=cache_[(zoom-1)*8+facing];
 if(cached.rgba){blit(cached.rgba,sw,sh,output,width,height,cx,foot,zoom);return true;}
 auto* pixels=(uint8_t*)malloc(cells);auto* depths=(uint16_t*)malloc(cells*sizeof(uint16_t));auto* before=(uint16_t*)malloc(cells*sizeof(uint16_t));auto* rgb=(uint8_t*)calloc(cells,3);
 const size_t total=(size_t)body_.voxel_count()+reactor_.voxel_count();
 if(total>100000){free(pixels);free(depths);free(before);free(rgb);return false;}
 auto* points=(Projected*)malloc(total*sizeof(Projected));
 if(!pixels || !depths || !before || !rgb || !points){free(pixels);free(depths);free(before);free(rgb);free(points);return false;}
 bool ok=true;size_t at=0;float min_x=1e30f,max_x=-1e30f,max_y=-1e30f;
 const float angle=(float)((facing+5)&7)*.7853749990463257f,co=cosf(angle),si=sinf(angle);
 // Original flat camera tilt and source normal-zoom scale, adjusted to this
 // viewer's 64-pixel tile rather than the original 100-pixel tile basis.
 const float scale=.07000000029802322f*1.25f*(64.0f/100.0f)*zoom;
 for(unsigned component=0;component<2;++component){const CvrModel& model=component?reactor_:body_;
  for(unsigned i=0;i<model.object_count();++i){const auto*o=model.object(i);const auto&f=o->frames[0];if(f.flags&0x80)continue;
   for(unsigned j=0;j<o->count;++j){const auto&v=o->voxels[j];float local[3]={v.z*o->units,v.x*o->units,v.y*o->units},world[3];
    for(unsigned a=0;a<3;++a){world[a]=f.location[a];for(unsigned b=0;b<3;++b)world[a]+=f.matrix[a*3+b]*local[b];}
    world[0]-=400;const float side=world[0]*co-world[1]*si,along=world[0]*si+world[1]*co;
    Projected p{side,along*.500459611415863f-world[2]*.8657598495483398f,-along*.8657598495483398f-world[2]*.500459611415863f,v.normal,v.color};
    if(!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z) || fabsf(p.x)>100000 || fabsf(p.y)>100000 || fabsf(p.z)>100000){ok=false;break;}
    points[at++]=p;min_x=fminf(min_x,p.x);max_x=fmaxf(max_x,p.x);max_y=fmaxf(max_y,p.y);
   }if(!ok)break;
  }if(!ok)break;
 }
 if(!at)ok=false;
 for(size_t i=0;i<cells;++i)depths[i]=65535;
 const float origin_x=sw*.5f-(min_x+max_x)*.5f*scale,origin_y=(float)sh-8*zoom-max_y*scale;
 alpha::CvrRasterSurface surface{pixels,depths,cells,sw,sh,sw};alpha::CvrRasterClip clip{0,0,(int32_t)sw-1,(int32_t)sh-1};
 const float view_light=.5773500204086304f;
 const float light[3]={-view_light,(.500459611415863f-.8657598495483398f)*view_light,-(.8657598495483398f+.500459611415863f)*view_light};
 at=0;
 for(unsigned component=0;ok && component<2;++component){const CvrModel&model=component?reactor_:body_;uint8_t palette[6144];for(unsigned s=0;s<24;++s)for(unsigned c=0;c<256;++c)palette[s*256+c]=model.shaded_index((uint8_t)c,s);
  for(unsigned i=0;ok && i<model.object_count();++i){const auto*o=model.object(i);const auto&f=o->frames[0];if(f.flags&0x80)continue;
   float pose[9];for(unsigned col=0;col<3;++col){pose[col]=co*f.matrix[col]-si*f.matrix[3+col];pose[3+col]=si*f.matrix[col]+co*f.matrix[3+col];pose[6+col]=f.matrix[6+col];}
   float object_light[3];uint8_t shades[2048];if(!inverse_light(pose,light,object_light) || !cvr_shades_for_light(lattice_,cvr_shade_lattice_size,object_light[0],object_light[1],object_light[2],shades,sizeof(shades))){ok=false;break;}
   auto*batch=(CvrRasterPoint*)malloc((size_t)o->count*sizeof(CvrRasterPoint));if(!batch){ok=false;break;}
   for(unsigned j=0;j<o->count;++j){const auto&p=points[at++];const float x=(origin_x+p.x*scale)*256,y=(origin_y+p.y*scale)*256,z=32768+p.z*16;
    if(x<INT32_MIN || x>=2147483648.0f || y<INT32_MIN || y>=2147483648.0f || z<0 || z>=65535){ok=false;break;}
    batch[j]={(int32_t)x,(int32_t)y,(uint32_t)z<<16,p.normal,p.color};}
   memcpy(before,depths,cells*sizeof(uint16_t));
   if(ok)ok=raster_cvr_points(batch,o->count,shades,sizeof(shades),palette,sizeof(palette),clip,scale*o->units>.5f,surface);
   free(batch);
   if(ok)for(size_t j=0;j<cells;++j)if(depths[j]!=before[j])memcpy(rgb+j*3,model.palette_rgb(pixels[j]),3);
  }
 }
 // Commit the private surface only after all model/transformation checks pass.
 if(ok){cached.rgba=(uint8_t*)malloc(cells*4);if(cached.rgba){for(size_t i=0;i<cells;++i){memcpy(cached.rgba+i*4,rgb+i*3,3);cached.rgba[i*4+3]=depths[i]!=65535;}blit(cached.rgba,sw,sh,output,width,height,cx,foot,zoom);}}
 if(ok && !cached.rgba)for(unsigned y=0;y<sh;++y)for(unsigned x=0;x<sw;++x){const size_t n=(size_t)y*sw+x;const int dx=cx-(int)sw/2+(int)x,dy=foot-(int)sh+8*(int)zoom+(int)y;if(depths[n]!=65535 && dx>=0 && dy>=0 && dx<(int)width && dy<(int)height)memcpy(output+((size_t)dy*width+dx)*3,rgb+n*3,3);}
 free(pixels);free(depths);free(before);free(rgb);free(points);return ok;
}
}
