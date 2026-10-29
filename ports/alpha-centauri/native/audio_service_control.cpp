/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host control for the SDL audio service through SDL's "disk" output driver.
 * Usage: SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=OUT audio-service-control DATA_DIR OUT
 * DATA_DIR holds synthetic fx/, voices/ and music/ WAVs written by test_audio_service.py.
 */
#include "audio_service.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static unsigned checks,failures;
static bool check(bool good,const char* what){++checks;if(!good){++failures;printf("FAIL %u %s\n",checks,what);}return good;}
#define CHECK(x) check((x),#x)
static size_t count_value(const int16_t* s,size_t n,int v){size_t k=0;for(size_t i=0;i<n;++i)k+=s[i]==v;return k;}
int main(int argc,char** argv){
 if(argc!=3)return 20;
 // Every call is harmless before the service exists.
 audio_event(AudioEvent::ui_click);CHECK(!audio_faction_speech("gaians",false)&&!audio_ready());audio_suspend();CHECK(!audio_resume());audio_close();
 if(SDL_Init(0))return 20;
 char cfg[512];snprintf(cfg,sizeof(cfg),"%s/alpha-audio.cfg",argv[1]);
 CHECK(audio_open(argv[1],cfg)&&audio_ready());
 const AudioSettings& s=audio_settings();
 CHECK(s.levels.music==100&&s.levels.master==100&&s.levels.music_duck==40);
 CHECK(audio_world_music("peace")); // gset1 per the original faction table
 SDL_Delay(300);
 // Suspend/resume keeps playlist and stream (the disk driver restarts its file).
 auto before=audio_status();audio_suspend();CHECK(!audio_ready());SDL_Delay(50);CHECK(audio_resume()&&audio_ready());
 auto after=audio_status();CHECK(before.tracks_started==after.tracks_started&&after.tracks_started==1);
 SDL_Delay(300);
 audio_event(AudioEvent::ui_click);   // fx/ok.wav, constant 1000
 SDL_Delay(300);
 audio_event(AudioEvent::unit_move);  // mapped to a missing file: counted, silent
 audio_event(AudioEvent::unit_move);  // negative cache: not counted twice
 CHECK(audio_faction_speech("gaians",false)); // voices/gaians.wav, constant 3000
 CHECK(!audio_faction_speech("cyborg",false)&&!audio_tech_discovered(87,true)&&!audio_facility_complete(1,true));
 SDL_Delay(500);
 auto mid=audio_status();CHECK(mid.missing_files>=2&&mid.cached_clips==1&&mid.music_underruns==0);
 audio_interlude(3);SDL_Delay(2300);CHECK(!audio_status().voice_playing);audio_interlude_closed();
 AudioSettings off=audio_settings();off.levels.music_on=false;audio_set_settings(off);SDL_Delay(400);
 char saved[512];snprintf(saved,sizeof(saved),"%s/saved.cfg",argv[1]);CHECK(audio_save_settings(saved));
 audio_close();CHECK(!audio_ready());
 SDL_Quit();
 FILE* f=fopen(argv[2],"rb");if(!check(f!=nullptr,"disk output"))return 20;
 static int16_t pcm[1<<20];size_t n=fread(pcm,2,sizeof(pcm)/2,f);fclose(f);
 size_t music=count_value(pcm,n,500),click=count_value(pcm,n,1500),voice=count_value(pcm,n,3000+((500*1638)>>12)); // music ducked to 40% (Q12 gain 1638)
 printf("SERVICE OUTPUT samples=%lu music=%lu music+click=%lu voice+ducked=%lu tail=%d\n",(unsigned long)n,(unsigned long)music,(unsigned long)click,(unsigned long)voice,n?pcm[n-1]:-1);
 CHECK(music>4000&&click==2205&&voice>4000&&n&&pcm[n-1]==0);
 FILE* g=fopen(saved,"rb");char text[2048]={0};if(g){size_t k=fread(text,1,sizeof(text)-1,g);text[k]=0;fclose(g);}
 CHECK(strstr(text,"music=off\n")&&strstr(text,"map.unit_move=missing.wav\n"));
 printf("AUDIO SERVICE %u checks %s\n",checks,failures?"FAIL":"PASS");return failures?20:0;
}
