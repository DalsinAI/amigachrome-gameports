/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "leader_speech.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const int16_t* pcm;static size_t remaining;
static void audio(void*,Uint8* stream,int bytes){
 size_t n=(size_t)bytes/2;if(n>remaining)n=remaining;
 memcpy(stream,pcm,n*2);memset(stream+n*2,0,(size_t)bytes-n*2);pcm+=n;remaining-=n;
}
int main(int argc,char** argv){
 if(argc==1){puts("Original faction-selection speeches (not dynamic diplomacy):");for(size_t i=0;i<14;++i){auto e=alpha::leader_speech(i);printf("%-8s  %s [%s]\n",e->stem,e->leader,e->expansion?"SMAX":"SMAC");}puts("Usage: leader-speech FACTION PRIVATE_MEDIA_DIRECTORY");return 0;}
 if(argc!=3)return 20;
 int index=alpha::find_leader_speech(argv[1],true);if(index<0){puts("Unknown faction basename");return 20;}
 char path[512];int len=snprintf(path,sizeof(path),"%s/voices/%s.wav",argv[2],alpha::leader_speech((size_t)index)->stem);if(len<0||(size_t)len>=sizeof(path))return 20;
 FILE* f=fopen(path,"rb");if(!f){printf("Missing privately prepared speech: %s\n",path);return 20;}
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<12||n>16*1024*1024){fclose(f);return 20;}
 auto raw=(uint8_t*)malloc((size_t)n);auto samples=(int16_t*)malloc((size_t)n);if(!raw||!samples){fclose(f);free(raw);free(samples);return 20;}
 bool okay=fread(raw,1,(size_t)n,f)==(size_t)n;fclose(f);size_t count=0;okay=okay&&alpha::decode_speech_wav(raw,(size_t)n,samples,(size_t)n/2,count);free(raw);if(!okay){free(samples);puts("Invalid speech PCM profile");return 20;}
 if(SDL_Init(SDL_INIT_AUDIO)){free(samples);return 20;}
 SDL_AudioSpec want={};want.freq=22050;want.format=AUDIO_S16SYS;want.channels=1;want.samples=1024;want.callback=audio;
 SDL_AudioDeviceID device=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);if(!device){printf("Audio: %s\n",SDL_GetError());SDL_Quit();free(samples);return 20;}
 pcm=samples;remaining=count;printf("SPEECH %s samples=%lu\n",alpha::leader_speech((size_t)index)->stem,(unsigned long)count);fflush(stdout);SDL_PauseAudioDevice(device,0);
 for(;;){SDL_Delay(20);SDL_LockAudioDevice(device);bool done=remaining==0;SDL_UnlockAudioDevice(device);if(done)break;}
 SDL_Delay(100);SDL_CloseAudioDevice(device);SDL_Quit();free(samples);puts("SPEECH COMPLETE RC0");return 0;
}
