/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "battle_aftermath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
alignas(BattleAftermathPlan) static UnitDefinitions units{};
alignas(BattleAftermathPlan) static Rules tech{};
alignas(BattleAftermathPlan) static BasicRules basic{};
alignas(BattleAftermathPlan) static EngagementFacts engagement{EngagementObservation::supplied,0,0,false,{61,11}};
alignas(BattleAftermathPlan) static RemovalRuntime removal{RemovalObservation::supplied,false,false,false,{0,0,0,0},0,0,0,0};
alignas(BattleAftermathPlan) static CasualtyInputs casualty{1,1,{true,0},{true,0},{true,0}};
alignas(BattleAftermathPlan) static LandResolutionFacts resolution{engagement,removal,EngagementObservation::supplied,1,1,{{true,0},{true,0},{true,0}}};
static void poison(void*p,size_t n){
#ifdef __SANITIZE_ADDRESS__
 __asan_poison_memory_region(p,n);
#else
 (void)p;(void)n;
#endif
}
static void unpoison(void*p,size_t n){
#ifdef __SANITIZE_ADDRESS__
 __asan_unpoison_memory_region(p,n);
#else
 (void)p;(void)n;
#endif
}
static uint32_t hash(const void*p,size_t n){const auto*b=(const uint8_t*)p;uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f||fseek(f,0,SEEK_END))return 20;long n=ftell(f);if(n<0||fseek(f,0,SEEK_SET))return 20;auto*b=(uint8_t*)malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);OriginalWorld w;if(w.load(b,(size_t)n,123)!=WorldLoadError::ok||!w.vehicle(1))return 21;free(b);
for(unsigned stage=0;stage<3;++stage){void*inputs[4];size_t sizes[4];
 if(stage==2){inputs[0]=&removal;sizes[0]=sizeof removal;inputs[1]=&units;sizes[1]=sizeof units;inputs[2]=&tech;sizes[2]=sizeof tech;inputs[3]=&casualty;sizes[3]=sizeof casualty;}
 else{inputs[0]=&units;sizes[0]=sizeof units;inputs[1]=&tech;sizes[1]=sizeof tech;inputs[2]=&basic;sizes[2]=sizeof basic;if(stage==0){inputs[3]=&engagement;sizes[3]=sizeof engagement;}else{inputs[3]=&resolution;sizes[3]=sizeof resolution;}}
 for(unsigned j=0;j<4;++j){const auto before=hash(inputs[j],sizes[j]);poison(inputs[j],sizes[j]);bool rejected=false;
 if(stage==0)rejected=stage_land_engagement(w,units,tech,basic,0,1,engagement,*reinterpret_cast<LandEngagementPlan*>(inputs[j]))==EngagementError::bad_state;
 if(stage==1)rejected=stage_land_resolution(w,units,tech,basic,0,1,resolution,*reinterpret_cast<LandResolutionPlan*>(inputs[j]))==EngagementError::bad_state;
 if(stage==2)rejected=stage_artifact_capture(w,0,removal,units,tech,3,casualty,*reinterpret_cast<ArtifactCapturePlan*>(inputs[j]))==RemovalError::bad_state;
 unpoison(inputs[j],sizes[j]);if(!rejected||hash(inputs[j],sizes[j])!=before)return 22;
 const unsigned char row[4]={(unsigned char)stage,(unsigned char)j,0,0};if(fwrite(row,1,4,stdout)!=4)return 23;
 }
}return ferror(stdout)?24:0;}
