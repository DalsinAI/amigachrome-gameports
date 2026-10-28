/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_mixer.h"
#include <string.h>
namespace alpha {
namespace {
const int32_t unity=4096; // Q12 gain
int32_t percent(int32_t gain,uint8_t p){return gain*(p>100?100:p)/100;}
// Linear interpolation between a and b at 16-bit fraction f (kept in 32 bits).
inline int32_t lerp(int32_t a,int32_t b,uint32_t f){return a+(((b-a)*(int32_t)(f>>1))>>15);}
}
void AudioMixer::reset(){
 for(auto& c:sfx_)c=Channel{};
 voice_=Channel{};serial_=0;
 music_read_=music_write_=0;music_frac_=0;music_step_=1u<<16;music_live_=music_eos_=false;music_gain_now_=0;stats_=MixerStats{};
}
uint32_t AudioMixer::step_for(uint32_t rate){return (uint32_t)(((uint64_t)rate<<16)/output_rate);}
int32_t AudioMixer::bus_gain(uint8_t bus,bool on) const{
 if(!on)return 0;
 return percent(percent(unity,levels_.master),bus);
}
int AudioMixer::play_sfx(const AudioClip& clip,uint8_t gain){
 if(!clip.samples||!clip.frames||!clip.rate)return -1;
 int slot=-1;for(int i=0;i<sfx_slots;++i)if(!sfx_[i].clip.samples){slot=i;break;}
 if(slot<0){slot=0;for(int i=1;i<sfx_slots;++i)if(sfx_[i].serial<sfx_[slot].serial)slot=i;++stats_.sfx_stolen;}
 Channel& c=sfx_[slot];c=Channel{};c.clip=clip;c.step=step_for(clip.rate);c.gain=percent(unity,gain);c.serial=++serial_;return slot;
}
void AudioMixer::stop_sfx(){for(auto& c:sfx_)c=Channel{};}
bool AudioMixer::sfx_active() const{for(const auto& c:sfx_)if(c.clip.samples)return true;return false;}
void AudioMixer::play_voice(const AudioClip& clip){
 voice_=Channel{};if(!clip.samples||!clip.frames||!clip.rate)return;
 voice_.clip=clip;voice_.step=step_for(clip.rate);voice_.gain=unity;voice_.fade=unity<<8;voice_.serial=++serial_;
}
void AudioMixer::stop_voice(){voice_=Channel{};}
void AudioMixer::fade_voice(uint32_t ms){
 if(!voice_.clip.samples)return;
 uint32_t samples=(uint32_t)((uint64_t)ms*output_rate/1000);if(!samples){stop_voice();return;}
 int32_t step=(int32_t)((unity<<8)/samples);voice_.fade_step=step?step:1;
}
bool AudioMixer::references(const int16_t* p) const{
 if(!p)return false;
 if(voice_.clip.samples==p)return true;
 for(const auto& c:sfx_)if(c.clip.samples==p)return true;
 return false;
}
void AudioMixer::music_start(uint32_t rate){
 music_read_=music_write_=0;music_frac_=0;music_step_=step_for(rate?rate:output_rate);music_live_=true;music_eos_=false;
}
void AudioMixer::music_stop(){music_live_=false;music_eos_=false;music_read_=music_write_=0;music_frac_=0;}
size_t AudioMixer::music_push(const int16_t* s,size_t n){
 size_t room=music_space();if(n>room)n=room;
 for(size_t i=0;i<n;++i){music_[music_write_]=s[i];music_write_=(music_write_+1)%music_ring;}
 return n;
}
// Returns false once the clip has finished.
bool AudioMixer::mix_channel(Channel& c,int32_t* acc,size_t frames,int32_t gain){
 const int16_t* s=c.clip.samples;uint32_t n=c.clip.frames;
 for(size_t i=0;i<frames;++i){
  if(c.pos>=n)return false;
  int32_t v=s[c.pos];
  if(c.frac)v=lerp(v,c.pos+1<n?s[c.pos+1]:0,c.frac);
  int32_t g=gain;
  if(c.fade_step){c.fade-=c.fade_step;if(c.fade<=0)return false;g=(int32_t)(((int64_t)g*(c.fade>>8))>>12);}
  acc[i]+=g==unity?v:(v*g)>>12;
  uint32_t f=c.frac+c.step;c.pos+=f>>16;c.frac=f&0xFFFF;
 }
 return c.pos<n;
}
void AudioMixer::render(int16_t* out,size_t frames){
 const int32_t sfx_bus=bus_gain(levels_.sfx,levels_.sfx_on),voice_bus=bus_gain(levels_.voice,levels_.voice_on);
 int32_t music_target=bus_gain(levels_.music,levels_.music_on);
 if(voice_.clip.samples && voice_bus)music_target=percent(music_target,levels_.music_duck);
 while(frames){
  size_t n=frames<512?frames:512;memset(acc_,0,n*sizeof(int32_t));
  for(auto& c:sfx_)if(c.clip.samples){int32_t g=(int32_t)(((int64_t)sfx_bus*c.gain)>>12);if(!mix_channel(c,acc_,n,g))c=Channel{};}
  if(voice_.clip.samples && !mix_channel(voice_,acc_,n,voice_bus))voice_=Channel{};
  // A muted music bus pauses the stream (position kept) once its ramp ends.
  if(music_live_ && (music_target || music_gain_now_)){
   const size_t need=2+(music_step_>>16); // both interpolation taps plus the advance stay unread-safe
   for(size_t i=0;i<n;++i){
    if(music_gain_now_!=music_target){int32_t d=music_target-music_gain_now_;music_gain_now_+=d>8?8:d<-8?-8:d;} // ~23 ms full ramp
    size_t fill=music_fill();
    if(fill<need && !(music_eos_ && fill)){if(!music_eos_)++stats_.music_underruns;break;}
    // At the end of the stream the last samples play without a future tap.
    int32_t a=music_[music_read_],b=fill>=2?music_[(music_read_+1)%music_ring]:a;
    int32_t v=music_frac_?lerp(a,b,music_frac_):a;
    acc_[i]+=(v*music_gain_now_)>>12;
    uint32_t f=music_frac_+music_step_;size_t advance=f>>16;if(advance>fill)advance=fill;
    music_read_=(music_read_+advance)%music_ring;music_frac_=f&0xFFFF;
   }
  }
  for(size_t i=0;i<n;++i){int32_t v=acc_[i];if(v>32767){v=32767;++stats_.clipped;}else if(v<-32768){v=-32768;++stats_.clipped;}out[i]=(int16_t)v;}
  out+=n;frames-=n;stats_.rendered+=(uint32_t)n;
 }
}
}
