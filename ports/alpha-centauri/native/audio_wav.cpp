/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_wav.h"
#include <string.h>
namespace alpha {
namespace {
uint16_t u16(const uint8_t* p){return uint16_t(p[0]|p[1]<<8);}
int16_t s16(const uint8_t* p){uint16_t v=u16(p);return v<32768?(int16_t)v:(int16_t)((int32_t)v-65536);}
uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
int16_t clamp16(int32_t v){return v>32767?32767:v<-32768?-32768:(int16_t)v;}
const int ima_index[16]={-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
const int16_t ima_step[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
const int ms_adapt[16]={230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230};
const int16_t ms_default[7][2]={{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};
struct Ima {int32_t predictor;int index;
 // ((2*magnitude+1)*step)>>3 without a multiply (matches the ffmpeg
 // adpcm_ima_wav reference bit-exactly; the shift-add IMA reference differs
 // only in rounding).
 int16_t step(unsigned nibble){int32_t s=ima_step[index],t=s;if(nibble&1)t+=s<<1;if(nibble&2)t+=s<<2;if(nibble&4)t+=s<<3;int32_t diff=t>>3;if(nibble&8)diff=-diff;
  predictor+=diff;if(predictor>32767)predictor=32767;else if(predictor<-32768)predictor=-32768;index+=ima_index[nibble];if(index<0)index=0;else if(index>88)index=88;return (int16_t)predictor;}
};
struct Ms {int c1,c2,delta;int32_t s1,s2;
 int16_t step(unsigned nibble){int32_t signed_nibble=nibble>=8?(int32_t)nibble-16:(int32_t)nibble;int32_t predicted=(s1*c1+s2*c2)/256+signed_nibble*delta;int16_t v=clamp16(predicted);s2=s1;s1=v;
  delta=(ms_adapt[nibble]*delta)/256;if(delta<16)delta=16;return v;}
};
// Frames a block (possibly a short final one of `bytes`) yields; 0 = unusable.
uint32_t block_frames(const WavInfo& w,size_t bytes){
 size_t ch=w.channels;
 if(w.codec==WavCodec::ima_adpcm){if(bytes<4*ch)return 0;size_t body=bytes-4*ch;if(ch==2)body-=body%8;size_t n=1+body*2/ch;return n>w.samples_per_block?w.samples_per_block:(uint32_t)n;}
 if(bytes<7*ch)return 0;
 size_t n=2+(bytes-7*ch)*2/ch;return n>w.samples_per_block?w.samples_per_block:(uint32_t)n;
}
}
const char* wav_error_name(WavError e){
 switch(e){case WavError::ok:return "ok";case WavError::truncated:return "truncated";case WavError::not_riff:return "not-riff";case WavError::bad_chunk:return "bad-chunk";case WavError::bad_format:return "bad-format";case WavError::unsupported:return "unsupported";case WavError::too_large:return "too-large";case WavError::capacity:return "capacity";case WavError::no_data:return "no-data";}
 return "?";
}
WavError parse_wav(const uint8_t* b,size_t available,size_t file_size,WavInfo& out){
 if(!b||available>file_size)return WavError::truncated;
 if(file_size>64u*1024u*1024u)return WavError::too_large;
 if(available<12)return WavError::truncated;
 if(memcmp(b,"RIFF",4)||memcmp(b+8,"WAVE",4))return WavError::not_riff;
 // Some shipped WAVs carry a stale RIFF size; chunks are bounded by the file.
 size_t end=file_size;uint32_t riff=u32(b+4);if((size_t)riff+8<end && riff>=4)end=(size_t)riff+8;
 WavInfo w;memset(&w,0,sizeof(w));bool fmt=false,data=false;
 for(size_t pos=12;pos+8<=end && !data;){
  if(pos+8>available)return WavError::truncated;
  const uint8_t* h=b+pos;size_t len=u32(h+4);pos+=8;
  if(!memcmp(h,"fmt ",4)){
   if(fmt)return WavError::bad_chunk;
   if(len<16||len>end-pos)return WavError::bad_chunk;
   if(pos+len>available)return WavError::truncated;
   const uint8_t* f=b+pos;uint16_t tag=u16(f),bits=u16(f+14);w.channels=u16(f+2);w.rate=u32(f+4);w.block_align=u16(f+12);
   if(w.channels<1||w.channels>2||w.rate<4000||w.rate>48000||!w.block_align)return WavError::bad_format;
   if(tag==0xFFFE){if(len<40||u16(f+24)!=1)return WavError::unsupported;tag=1;}
   if(tag==1){
    if(bits==8)w.codec=WavCodec::pcm_u8;else if(bits==16)w.codec=WavCodec::pcm_s16;else return WavError::unsupported;
    if(w.block_align!=w.channels*bits/8)return WavError::bad_format;
   }else if(tag==0x11){
    if(bits!=4||len<20||w.block_align<4*w.channels+1||w.block_align>8192)return WavError::bad_format;
    w.codec=WavCodec::ima_adpcm;uint32_t computed=(uint32_t)(w.block_align-4*w.channels)*8/(4*w.channels)+1;
    w.samples_per_block=len>=20&&u16(f+16)>=2?u16(f+18):(uint16_t)computed;if(w.samples_per_block!=computed)return WavError::bad_format;
    if(w.channels==2&&(w.block_align-8)%8)return WavError::bad_format;
   }else if(tag==2){
    if(bits!=4||len<22||w.block_align<7*w.channels+1||w.block_align>8192)return WavError::bad_format;
    w.codec=WavCodec::ms_adpcm;w.samples_per_block=u16(f+18);w.coefficient_count=u16(f+20);
    uint32_t computed=(uint32_t)(w.block_align-7*w.channels)*8/(4*w.channels)+2;
    if(w.samples_per_block<2||w.samples_per_block>computed||w.coefficient_count<7||w.coefficient_count>32||len<22+4u*w.coefficient_count)return WavError::bad_format;
    for(unsigned i=0;i<w.coefficient_count;++i){w.coefficients[i][0]=s16(f+22+4*i);w.coefficients[i][1]=s16(f+24+4*i);}
    for(unsigned i=0;i<7;++i)if(w.coefficients[i][0]!=ms_default[i][0]||w.coefficients[i][1]!=ms_default[i][1])return WavError::bad_format;
   }else return WavError::unsupported;
   fmt=true;pos+=len;if(len&1){if(pos<end)++pos;}
  }else if(!memcmp(h,"data",4)){
   if(!fmt)return WavError::bad_chunk;
   size_t have=end-pos;if(len>have){w.clamped=true;len=have;}
   w.data_offset=pos;data=true;
   if(w.codec==WavCodec::pcm_u8||w.codec==WavCodec::pcm_s16){size_t fb=wav_frame_bytes(w);len-=len%fb;if(len/fb>0xFFFFFFFFu)return WavError::too_large;w.frames=(uint32_t)(len/fb);}
   else{size_t blocks=len/w.block_align,rest=len%w.block_align;uint64_t n=(uint64_t)blocks*w.samples_per_block;uint32_t tail=rest?block_frames(w,rest):0;if(!tail)len-=rest;n+=tail;if(n>0xFFFFFFFFu)return WavError::too_large;w.frames=(uint32_t)n;}
   w.data_bytes=len;if(!w.frames)return WavError::no_data;
  }else{
   if(len>end-pos)return WavError::bad_chunk;
   pos+=len;if(len&1){if(pos<end)++pos;}
  }
 }
 if(!fmt)return WavError::bad_chunk;
 if(!data)return WavError::no_data;
 out=w;return WavError::ok;
}
bool convert_pcm_frames(const uint8_t* raw,size_t frames,const WavInfo& w,int16_t* out){
 if(w.codec==WavCodec::pcm_u8){
  if(w.channels==1)for(size_t i=0;i<frames;++i)out[i]=(int16_t)(((int)raw[i]-128)*256);
  else for(size_t i=0;i<frames;++i)out[i]=(int16_t)(((int)raw[2*i]+(int)raw[2*i+1]-256)*128);
  return true;
 }
 if(w.codec==WavCodec::pcm_s16){
  if(w.channels==1)for(size_t i=0;i<frames;++i)out[i]=s16(raw+2*i);
  else for(size_t i=0;i<frames;++i)out[i]=(int16_t)(((int32_t)s16(raw+4*i)+s16(raw+4*i+2))>>1);
  return true;
 }
 return false;
}
WavError decode_wav_mono(const uint8_t* b,size_t size,const WavInfo& w,int16_t* out,size_t capacity,size_t& count){
 if(!b||!out||w.data_offset>size||w.data_bytes>size-w.data_offset)return WavError::truncated;
 if(w.frames>capacity)return WavError::capacity;
 const uint8_t* d=b+w.data_offset;
 if(convert_pcm_frames(d,w.frames,w,out)){count=w.frames;return WavError::ok;}
 size_t produced=0,ch=w.channels;
 for(size_t at=0;at<w.data_bytes;at+=w.block_align){
  size_t bytes=w.data_bytes-at<w.block_align?w.data_bytes-at:w.block_align;const uint8_t* blk=d+at;uint32_t n=block_frames(w,bytes);
  if(!n||produced+n>w.frames)return WavError::bad_format;
  int16_t* o=out+produced;
  if(w.codec==WavCodec::ima_adpcm){
   Ima st[2];for(size_t c=0;c<ch;++c){st[c].predictor=s16(blk+4*c);st[c].index=blk[4*c+2];if(st[c].index>88)return WavError::bad_format;}
   if(ch==1){o[0]=(int16_t)st[0].predictor;for(uint32_t i=1;i<n;++i){uint8_t byte=blk[4+(i-1)/2];o[i]=st[0].step((i-1)&1?byte>>4:byte&15);}}
   else{
    o[0]=(int16_t)((st[0].predictor+st[1].predictor)>>1);
    int16_t left[8],right[8];
    for(uint32_t g=0;1+8*g<n;++g){const uint8_t* q=blk+8+8*g; // 4 bytes left, then 4 bytes right
     for(int k=0;k<8;++k){left[k]=st[0].step(k&1?q[k/2]>>4:q[k/2]&15);right[k]=st[1].step(k&1?q[4+k/2]>>4:q[4+k/2]&15);}
     for(uint32_t k=0;k<8 && 1+8*g+k<n;++k)o[1+8*g+k]=(int16_t)(((int32_t)left[k]+right[k])>>1);
    }
   }
  }else{
   Ms st[2];
   for(size_t c=0;c<ch;++c){unsigned p=blk[c];if(p>=w.coefficient_count)return WavError::bad_format;st[c].c1=w.coefficients[p][0];st[c].c2=w.coefficients[p][1];
    st[c].delta=s16(blk+ch+2*c);st[c].s1=s16(blk+3*ch+2*c);st[c].s2=s16(blk+5*ch+2*c);}
   if(ch==1){o[0]=(int16_t)st[0].s2;o[1]=(int16_t)st[0].s1;}
   else{o[0]=(int16_t)((st[0].s2+st[1].s2)>>1);o[1]=(int16_t)((st[0].s1+st[1].s1)>>1);}
   const uint8_t* q=blk+7*ch;
   if(ch==1)for(uint32_t i=2;i<n;++i){uint8_t byte=q[(i-2)/2];o[i]=st[0].step(i&1?byte&15:byte>>4);}
   else for(uint32_t i=2;i<n;++i){uint8_t byte=q[i-2];int16_t l=st[0].step(byte>>4),r=st[1].step(byte&15);o[i]=(int16_t)(((int32_t)l+r)>>1);}
  }
  produced+=n;
 }
 if(produced!=w.frames)return WavError::bad_format;
 count=produced;return WavError::ok;
}
size_t resampled_frames(size_t n,uint32_t from,uint32_t to){
 if(!n||!from||!to)return 0;
 return (size_t)(((uint64_t)n*to+from-1)/from);
}
size_t resample_linear(const int16_t* in,size_t n,uint32_t from,int16_t* out,size_t cap,uint32_t to){
 size_t m=resampled_frames(n,from,to);if(!m||m>cap||!in||!out)return 0;
 if(from==to){memcpy(out,in,n*2);return n;}
 const uint32_t step=(uint32_t)(((uint64_t)from<<16)/to);uint32_t pos=0,frac=0;
 for(size_t i=0;i<m;++i){
  int32_t a=pos<n?in[pos]:0,b=pos+1<n?in[pos+1]:a;
  out[i]=(int16_t)(a+(((b-a)*(int32_t)(frac>>1))>>15));
  uint32_t f=frac+step;pos+=f>>16;frac=f&0xFFFF;
 }
 return m;
}
}
