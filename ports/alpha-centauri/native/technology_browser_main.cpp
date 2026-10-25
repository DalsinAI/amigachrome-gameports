/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SDL_MAIN_HANDLED
#include "technology_browser.h"
#include <SDL2/SDL.h>
#include <exec/types.h>
#include <proto/exec.h>
struct IntuitionBase* IntuitionBase;struct GfxBase* GfxBase;struct Library* CyberGfxBase;
int main(int argc,char** argv){if(argc!=2)return 20;IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);int rc=20;
 if(IntuitionBase && GfxBase && CyberGfxBase && SDL_Init(SDL_INIT_AUDIO)==0)rc=alpha_technology_browser(argv[1]);
 SDL_Quit();if(CyberGfxBase)CloseLibrary(CyberGfxBase);if(GfxBase)CloseLibrary((struct Library*)GfxBase);if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);return rc;}
