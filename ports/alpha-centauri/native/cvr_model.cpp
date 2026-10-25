/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent implementation from published CVR chunk/layout facts and bounded
 * original-file observations. No CVR-Colorizer (GPL2-only) code is incorporated.
 */
#include "cvr_model.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128,"CVR requires binary32");
namespace alpha { namespace {
uint16_t u16(const uint8_t* p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
int16_t s16(const uint8_t* p){unsigned v=u16(p);return v<32768?(int16_t)v:(int16_t)((int)v-65536);}
uint32_t u32(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
bool real(const uint8_t* p,float& v){uint32_t bits=u32(p);memcpy(&v,&bits,4);return v==v && v>=-1000000.0f && v<=1000000.0f;}
struct Chunk {uint32_t tag;const uint8_t* data;size_t size;};
struct Cursor {
 const uint8_t* p;size_t size;
 bool next(Chunk& c){if(size<8)return false;uint32_t n=u32(p+4);if(n<8 || n>size)return false;c={u32(p),p+8,n-8};p+=n;size-=n;return true;}
};
bool name(const Chunk& c,char* out){if(!c.size || c.size>=64)return false;size_t n=c.size;if(c.data[n-1]==0)--n;for(size_t i=0;i<n;++i)if(c.data[i]<32 || c.data[i]>126)return false;memcpy(out,c.data,n);out[n]=0;return true;}
bool geometry(const Chunk& c,CvrObject& o,uint32_t& total){
 // Only observed single geometry headers and offset-free jump groups.
 if(c.size<50 || (u32(c.data)!=8 && u32(c.data)!=10) || u32(c.data+4)!=c.size || u32(c.data+12)!=c.size || u32(c.data+16)!=c.size-1)return false;
 if(!real(c.data+8,o.units) || o.units<=0 || o.units>1024)return false;
 uint32_t declared=u32(c.data+38),flags=u32(c.data);
 if(!declared || declared>1000000)return false;
 uint32_t capacity=flags==8?declared-1:declared;
 if(!capacity || total>2000000-capacity)return false;
 if(flags==8 && c.size!=42+(size_t)declared*3+5)return false;
 for(size_t i=c.size-5;i<c.size;++i)if(c.data[i])return false;
 int scale[3],pos[3],at[3];
 for(int i=0;i<3;++i){scale[i]=s16(c.data+20+2*i);pos[i]=s16(c.data+26+2*i);at[i]=s16(c.data+32+2*i);if(scale[i]<=0)return false;}
 o.voxels=(CvrVoxel*)malloc((size_t)capacity*sizeof(CvrVoxel));if(!o.voxels)return false;
 size_t offset=42,end=c.size-5;uint32_t groups=0;
 while(offset<end){
  uint32_t entries=declared;
  if(flags==10){
   if(end-offset<32)return false;
   const auto* jump=c.data+offset;entries=u32(jump+24);
   // Linked/offset jump headers remain unsupported until independently qualified.
   if(u32(jump+28)!=0)return false;
   for(int j=0;j<3;++j)at[j]=(int)s16(jump+2*j)+s16(jump+18+2*j);
   offset+=32;
  }
  if(++groups>4096 || !entries || entries>(end-offset)/3)return false;
  for(uint32_t i=0;i<entries;++i){
   const uint8_t* v=c.data+offset+(size_t)i*3;unsigned direction=v[0]>>3,normal=((unsigned)(v[0]&7)<<8)|v[1];
   if(direction>26 || (direction==26)!=(i==entries-1))return false;
   if(direction==26)continue;
   unsigned d=direction>=13?direction+1:direction;
   at[0]+=(int)(d/9)-1;at[1]+=(int)((d/3)%3)-1;at[2]+=(int)(d%3)-1;
   for(int j=0;j<3;++j)if(at[j]<pos[j] || at[j]>=pos[j]+scale[j] || at[j]<-32768 || at[j]>32767)return false;
   if(o.count>=capacity)return false;
   o.voxels[o.count++]={(int16_t)at[0],(int16_t)at[1],(int16_t)at[2],(uint16_t)normal,v[2]};
  }
  offset+=(size_t)entries*3;
 }
 if(offset!=end || o.count!=capacity)return false;
 total+=o.count;return true;
}
bool animation(const Chunk& c,CvrObject& o,uint32_t frames){
 o.frames=(CvrFrame*)calloc(frames,sizeof(CvrFrame));if(!o.frames)return false;
 for(uint32_t i=0;i<frames;++i)for(unsigned j=0;j<9;++j)o.frames[i].matrix[j]=j%4==0?1.0f:0.0f;
 Cursor r{c.data,c.size};Chunk v{};unsigned seen=0;
 while(r.size){if(!r.next(v))return false;unsigned bit=0;
  if(v.tag==0x04040301)bit=1;else if(v.tag==0x04040302)bit=2;else if(v.tag==0x04040303)bit=4;else return false;
  if(seen&bit)return false;
  seen|=bit;
  if(bit==1){if(v.size!=frames)return false;for(uint32_t i=0;i<frames;++i)o.frames[i].flags=v.data[i];}
  else {unsigned n=bit==2?3:9;if(v.size!=(size_t)frames*n*4)return false;for(uint32_t i=0;i<frames;++i)for(unsigned j=0;j<n;++j){float value;if(!real(v.data+((size_t)i*n+j)*4,value))return false;if(bit==2)o.frames[i].location[j]=value;else o.frames[i].matrix[j]=value;}}
 }
 return true;
}
bool parse_object(const Chunk& c,CvrObject& o,uint32_t frames,uint32_t& total){
 Cursor r{c.data,c.size};Chunk v{};unsigned seen=0;
 while(r.size){if(!r.next(v))return false;
  if(v.tag==0x04040100){if(seen&1 || !name(v,o.name))return false;seen|=1;}
  else if(v.tag==0x04040200){if(seen&2)return false;seen|=2;Cursor g{v.data,v.size};Chunk geom{};if(!g.next(geom) || g.size || geom.tag!=0x04040201 || !geometry(geom,o,total))return false;}
  else if(v.tag==0x04040300){if(seen&4 || !animation(v,o,frames))return false;seen|=4;}
  else return false;
 }
 return (seen&5)==5;
}
}
CvrModel::CvrModel():items_(nullptr),objects_(0),frames_(0),voxels_(0){memset(palette_,0,sizeof(palette_));memset(shades_,0,sizeof(shades_));}
CvrModel::~CvrModel(){clear();}
void CvrModel::clear(){if(items_)for(uint32_t i=0;i<objects_;++i){free(items_[i].voxels);free(items_[i].frames);}free(items_);items_=nullptr;objects_=frames_=voxels_=0;}
const uint8_t* CvrModel::rgb(uint8_t color,unsigned shade)const{return shade<24?palette_+3*shades_[shade*256+color]:nullptr;}
bool CvrModel::load(const void* data,size_t size){
 if(!data || size>16*1024*1024 || size<8)return false;
 CvrModel tmp;if(!tmp.parse((const uint8_t*)data,size))return false;
 clear();items_=tmp.items_;objects_=tmp.objects_;frames_=tmp.frames_;voxels_=tmp.voxels_;memcpy(palette_,tmp.palette_,sizeof(palette_));memcpy(shades_,tmp.shades_,sizeof(shades_));tmp.items_=nullptr;tmp.objects_=0;return true;
}
bool CvrModel::parse(const uint8_t* data,size_t size){
 Cursor all{data,size};Chunk root{};if(!all.next(root) || all.size || root.tag!=0x00525643)return false;
 Cursor r{root.data,root.size};Chunk c{};unsigned seen=0;
 while(r.size){if(!r.next(c))return false;
  if(c.tag==0x01000000){if(seen&1 || c.size!=4 || (u32(c.data)!=0x4121999au && u32(c.data)!=0x41233333u))return false;seen|=1;}
  else if(c.tag==0x02000000){char ignored[64];if(seen&2 || !name(c,ignored))return false;seen|=2;}
  else if(c.tag==0x03000000){
   if(seen&4)return false;
   seen|=4;Cursor p{c.data,c.size};Chunk v{};unsigned fields=0;
   while(p.size){if(!p.next(v))return false;
    if(v.tag==0x01010000){char ignored[64];if(fields&1 || !name(v,ignored))return false;fields|=1;}
    else if(v.tag==0x01020000){if(fields&2 || v.size!=6914 || v.data[0]>v.data[1])return false;fields|=2;memcpy(palette_,v.data+2,768);memcpy(shades_,v.data+770,6144);}
    else if(v.tag==0x01030000){if(fields&4 || v.size!=12288)return false;fields|=4;}
    else return false;
   }if((fields&3)!=3)return false;
  }else if(c.tag==0x04000000){
   if(seen&8)return false;
   seen|=8;Cursor scene{c.data,c.size};Chunk v{};unsigned fields=0;uint32_t actual=0;
   while(scene.size){if(!scene.next(v))return false;
    if(v.tag==0x04010000){char ignored[64];if(fields&1 || !name(v,ignored))return false;fields|=1;}
    else if(v.tag==0x04020000){if(fields&2 || v.size!=4 || items_)return false;fields|=2;objects_=u32(v.data);if(!objects_ || objects_>256)return false;items_=(CvrObject*)calloc(objects_,sizeof(CvrObject));if(!items_)return false;}
    else if(v.tag==0x04030000){if(fields&4 || v.size!=4 || actual)return false;fields|=4;frames_=u32(v.data);if(!frames_ || frames_>256)return false;}
    else if(v.tag==0x04040000){if(!items_ || !frames_ || actual>=objects_ || !parse_object(v,items_[actual],frames_,voxels_))return false;++actual;}
    else return false;
   }if((fields&7)!=7 || actual!=objects_)return false;
  }else return false;
 }
 return seen==15;
}
}
