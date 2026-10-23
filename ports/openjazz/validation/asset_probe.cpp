// Copyright (c) 2026 Dalsin Limited. GPL-2.0-only.
// Exercises the pinned engine's real loaders without entering gameplay loops.
#include "SDL_wrapper.h"
#include "game/game.h"
#include "io/file.h"
#include "io/gfx/video.h"
#include "jj1/level/jj1level.h"
#include "jj1/scene/jj1scene.h"
#include "menu/menu.h"
#include "platforms/platforms.h"
#include "util.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
extern int checkOptions(int, char**);
extern void startUp(const char*, int, char**);
extern void shutDown();
int main(int argc, char** argv) {
    if(argc!=4) { std::fprintf(stderr,"Usage: asset-probe DATA menu|scene|level NAME\n");return 2; }
    platform=IPlatform::make();
    char mute[]="--mute",nolog[]="--no-logfile",verbose[]="--verbose",warn[]="warn";
    char* args[]={argv[0],mute,nolog,verbose,warn,argv[1]};
    int count=checkOptions(6,args);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK)!=0) {
        std::fprintf(stderr,"FAIL SDL %s\n",SDL_GetError());return 3;
    }
    bool started=false;int result=0;
    try {
        startUp(nullptr,count,args);started=true;
        if(!std::strcmp(argv[2],"menu")) { MainMenu menu; }
        else if(!std::strcmp(argv[2],"scene")) { JJ1Scene scene(argv[3]); }
        else if(!std::strcmp(argv[2],"level")) {
            LocalGame game(argv[3],difficultyType::Normal);
            JJ1Level loaded(&game,argv[3],false,false);
            // Canonical byte-at-a-time digest of the loaded collision map.
            // No raw structs, pointers or host-endian bytes enter the hash.
            uint32_t hash=2166136261u;
            for(int y=0;y<LH*4;y++) for(int x=0;x<LW*4;x++) {
                hash^=loaded.checkMaskUp(ITOF(x*8),ITOF(y*8))?1u:0u;
                hash*=16777619u;
            }
            std::printf("STATE %s world=%d collision=%08lx\n",argv[3],loaded.getWorld(),static_cast<unsigned long>(hash));
        } else { std::fprintf(stderr,"FAIL unknown test kind\n");result=2; }
        if(!result)std::printf("PASS %s %s\n",argv[2],argv[3]);
    } catch(int e) { std::fprintf(stderr,"FAIL %s %s engine_error=%d\n",argv[2],argv[3],e);result=4; }
      catch(...) { std::fprintf(stderr,"FAIL %s %s unexpected exception\n",argv[2],argv[3]);result=5; }
    if(started)shutDown();
    delete platform;SDL_Quit();return result;
}
