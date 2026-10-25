/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original-asset title/menu and audio front end for the restoration preview.
 */
#define SDL_MAIN_HANDLED
#include "pcx_image.h"
#include "technology_browser.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/asl.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern struct IntuitionBase* IntuitionBase;extern struct GfxBase* GfxBase;extern struct Library* CyberGfxBase;
struct Library* AslBase;
int alpha_original_view(int argc,char** argv);
static alpha::PcxImage base_title,expansion_title;
static uint8_t* frame;static bool expansion=false,music_on=true;static SDL_AudioDeviceID audio_device;
static bool menu_audio_ready=false;
static uint8_t* audio_data;static Uint32 audio_length,audio_pos;
static char status[160]="Select a game; open a tutorial, scenario or save.";
static LONG text_pen=-1,border_pen=-1;static bool text_pen_owned=false,border_pen_owned=false;static TTF_Font* menu_font;static TTF_Font* detail_font;
static const int WIDTH=1024,HEIGHT=768;
static void music(void*,Uint8* stream,int length){
 if(length<=0)return;
 if(!audio_data || !audio_length){memset(stream,0,(size_t)length);return;}
 while(length){Uint32 n=audio_length-audio_pos;if(n>(Uint32)length)n=(Uint32)length;memcpy(stream,audio_data+audio_pos,n);stream+=n;length-=(int)n;audio_pos=(audio_pos+n)%audio_length;}
}
static void release_menu_audio(){if(audio_device)SDL_CloseAudioDevice(audio_device);audio_device=0;}
static void restore_menu_audio(){
 if(!menu_audio_ready)return;
 SDL_AudioSpec spec{};spec.freq=22050;spec.channels=1;spec.format=AUDIO_S16SYS;spec.samples=1024;spec.callback=music;
 audio_device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
 if(audio_device && music_on)SDL_PauseAudioDevice(audio_device,0);
}
static bool image(const char* name,alpha::PcxImage& img){
 FILE* f=fopen(name,"rb");if(!f)return false;if(fseek(f,0,SEEK_END)){fclose(f);return false;}long n=ftell(f);rewind(f);if(n<=0 || n>8*1024*1024){fclose(f);return false;}
 auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return false;}bool ok=fread(p,1,(size_t)n,f)==(size_t)n && img.load(p,(size_t)n);free(p);fclose(f);return ok && img.width()==WIDTH && img.height()==HEIGHT;
}
static void text(struct RastPort* rp,int x,int y,const char* message,bool detail=false){
 SDL_Color ink{238,221,170,255};
 SDL_Surface* rendered=TTF_RenderUTF8_Blended(detail?detail_font:menu_font,message,ink);
 if(!rendered)return;
 SDL_Surface* rgba=SDL_ConvertSurfaceFormat(rendered,SDL_PIXELFORMAT_RGBA32,0);
 SDL_FreeSurface(rendered);if(!rgba)return;
 int width=rgba->w;if(width>WIDTH-x)width=WIDTH-x;
 const int height=rgba->h,top=y-height;
 if(width<=0 || height<=0 || height>64 || top<0){SDL_FreeSurface(rgba);return;}
 auto* pixels=(uint8_t*)malloc((size_t)width*height*3);
 if(!pixels){SDL_FreeSurface(rgba);return;}
 ReadPixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);
 for(int row=0;row<height;++row)for(int col=0;col<width;++col){
  const auto* src=(const uint8_t*)rgba->pixels+(size_t)row*rgba->pitch+col*4;
  auto* dst=pixels+((size_t)row*width+col)*3;unsigned alpha=src[3];
  for(int channel=0;channel<3;++channel)dst[channel]=(uint8_t)((src[channel]*alpha+dst[channel]*(255-alpha)+127)/255);
 }
 WritePixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);
 free(pixels);SDL_FreeSurface(rgba);
}
static void button(struct Window* w,int x,int y,int width,const char* label,bool active=false){
 FillPixelArray(w->RPort,(UWORD)x,(UWORD)y,(UWORD)width,42,active?0x003b2c17u:0x00100905u);SetAPen(w->RPort,(ULONG)border_pen);Move(w->RPort,x,y);Draw(w->RPort,x+width-1,y);Draw(w->RPort,x+width-1,y+41);Draw(w->RPort,x,y+41);Draw(w->RPort,x,y);
 SetAPen(w->RPort,(ULONG)text_pen);SetDrMd(w->RPort,JAM1);text(w->RPort,x+18,y+27,label);
}
static void draw(struct Window* w){
 const auto& art=expansion?expansion_title:base_title;
 for(int y=0;y<HEIGHT;++y)for(int x=0;x<WIDTH;++x){const auto* rgb=art.rgb((uint32_t)x,(uint32_t)y);auto* p=frame+((size_t)y*WIDTH+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
 WritePixelArray(frame,0,0,WIDTH*3,w->RPort,10,24,WIDTH,HEIGHT,RECTFMT_RGB);
 button(w,45,78,220,"Alpha Centauri [A]",!expansion);button(w,277,78,230,"Alien Crossfire [X]",expansion);
 button(w,45,140,462,expansion?"Choose expansion scenario [E]":"Explore original tutorial [E]");
 button(w,45,192,462,"Open original scenario or save... [O]");
 button(w,45,244,462,audio_device?(music_on?"Original menu music: ON [M]":"Original menu music: OFF [M]"):"Original menu music unavailable");
 button(w,45,296,462,"Exit [Q]");
 button(w,45,348,462,"Technology discovery tree [T]");
 FillPixelArray(w->RPort,45,410,600,64,0x00100905u);SetAPen(w->RPort,(ULONG)text_pen);SetDrMd(w->RPort,JAM1);text(w->RPort,58,434,status,true);
 text(w->RPort,58,457,"Read-only preview; campaign restoration in progress.",true);
 printf("FRONTEND variant=%s music=%d device=%lu\n",expansion?"expansion":"base",music_on,(unsigned long)audio_device);fflush(stdout);
}
static void open_map(struct Window* w,const char* path){
 char* args[6]={const_cast<char*>("alpha-original-view"),const_cast<char*>(expansion?"expansion":"base"),const_cast<char*>(expansion?"alphax.txt":"alpha.txt"),const_cast<char*>(path),const_cast<char*>("texture.pcx"),const_cast<char*>("Units.pcx")};
 // The opening-menu loop belongs to this screen. Do not play it as a
 // substitute for the world's not-yet-restored ambient music system.
 release_menu_audio();
 int rc=alpha_original_view(6,args);
 restore_menu_audio();
 snprintf(status,sizeof(status),rc==0?"Returned from the original world view.":"Could not open that game/rules profile or a required original asset.");printf("FRONTEND MAP RC %d music_resumed=%d\n",rc,audio_device && music_on);fflush(stdout);ActivateWindow(w);draw(w);
}
static void choose_map(struct Window* w){
 if(!AslBase){snprintf(status,sizeof(status),"The file requester library is unavailable.");draw(w);return;}
 auto* req=(struct FileRequester*)AllocAslRequestTags(ASL_FileRequest,ASLFR_Window,(ULONG)w,ASLFR_TitleText,(ULONG)"Open an original Alpha scenario or save",ASLFR_InitialDrawer,(ULONG)"PROGDIR:",TAG_DONE);
 if(!req){snprintf(status,sizeof(status),"Could not open the file requester.");draw(w);return;}
 char path[1024];bool valid=false;
 if(AslRequestTags(req,TAG_DONE)){
  size_t n=strlen(req->fr_Drawer);int used=snprintf(path,sizeof(path),"%s%s%s",req->fr_Drawer,n && req->fr_Drawer[n-1]!=':' && req->fr_Drawer[n-1]!='/'?"/":"",req->fr_File);valid=used>0 && (size_t)used<sizeof(path);
 }
 FreeAslRequest(req);ActivateWindow(w);if(valid)open_map(w,path);else draw(w);
}
static void open_technologies(struct Window* w){
 release_menu_audio();
 int rc=alpha_technology_browser(expansion?"alphax.txt":"alpha.txt");
 restore_menu_audio();
 snprintf(status,sizeof(status),rc==0?"Returned from technology discovery tree.":"Technology rules/browser could not be opened.");
 printf("FRONTEND TECHNOLOGY RC %d music_resumed=%d\n",rc,audio_device && music_on);fflush(stdout);ActivateWindow(w);draw(w);
}
static void toggle_music(struct Window* w){if(audio_device){music_on=!music_on;SDL_PauseAudioDevice(audio_device,music_on?0:1);}draw(w);}
int main(int argc,char**){
 if(argc!=1){puts("Run alpha-restoration from the directory containing original game assets.");return 20;}
 if(!image("openinga.pcx",base_title) || !image("xopeninga.pcx",expansion_title)){puts("Original title PCXs are required.");return 20;}
 frame=(uint8_t*)malloc((size_t)WIDTH*HEIGHT*3);if(!frame)return 20;
 if(SDL_Init(SDL_INIT_AUDIO)==0){SDL_AudioSpec spec{};
  if(SDL_LoadWAV("opening menu.wav",&spec,&audio_data,&audio_length)){
   if(spec.format==AUDIO_S16LSB && spec.channels==1 && spec.freq==22050 && audio_length && !(audio_length&1u)){
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    for(Uint32 i=0;i<audio_length;i+=2){Uint8 a=audio_data[i];audio_data[i]=audio_data[i+1];audio_data[i+1]=a;}
#endif
    spec.format=AUDIO_S16SYS;spec.samples=1024;spec.callback=music;spec.userdata=nullptr;menu_audio_ready=true;audio_device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);if(audio_device)SDL_PauseAudioDevice(audio_device,0);
   }
  }
 }
 if(!audio_device)snprintf(status,sizeof(status),"Music: %s",SDL_GetError());
 if(TTF_Init()!=0){printf("Font initialization failed: %s\n",TTF_GetError());if(audio_device)SDL_CloseAudioDevice(audio_device);if(audio_data)SDL_FreeWAV(audio_data);SDL_Quit();free(frame);return 20;}
 menu_font=TTF_OpenFont("ALPHC___.TTF",20);detail_font=TTF_OpenFont("arialn.ttf",18);
 if(!menu_font || !detail_font){printf("Original game fonts are required: %s\n",TTF_GetError());if(menu_font)TTF_CloseFont(menu_font);if(detail_font)TTF_CloseFont(detail_font);TTF_Quit();if(audio_device)SDL_CloseAudioDevice(audio_device);if(audio_data)SDL_FreeWAV(audio_data);SDL_Quit();free(frame);return 20;}
 printf("FRONTEND FONTS Alpha Centauri 20; Arial Narrow 18\n");
 IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);AslBase=OpenLibrary((STRPTR)"asl.library",39);
 struct Window* w=nullptr;int rc=20;
 if(IntuitionBase && GfxBase && CyberGfxBase)w=OpenWindowTags(nullptr,WA_Title,(ULONG)"Alpha Centauri / Alien Crossfire - restoration preview",WA_Width,1044,WA_Height,800,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
 if(w){
  auto* colors=w->WScreen->ViewPort.ColorMap;text_pen=ObtainBestPen(colors,0xeeeeeeee,0xdddddddd,0xaaaaaaaa,TAG_DONE);border_pen=ObtainBestPen(colors,0xaaaaaaaau,0x88888888u,0x44444444u,TAG_DONE);text_pen_owned=text_pen>=0;border_pen_owned=border_pen>=0;if(text_pen<0)text_pen=1;if(border_pen<0)border_pen=1;
  draw(w);bool quit=false;
  while(!quit){WaitPort(w->UserPort);struct IntuiMessage* m;
   while((m=(struct IntuiMessage*)GetMsg(w->UserPort))){ULONG kind=m->Class;UWORD code=m->Code;WORD x=m->MouseX,y=m->MouseY;ReplyMsg((struct Message*)m);int action=0;
    if(kind==IDCMP_CLOSEWINDOW)quit=true;
    else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(w);draw(w);EndRefresh(w,TRUE);}
    else if(kind==IDCMP_VANILLAKEY){switch(code){case 'a':case 'A':action=1;break;case 'x':case 'X':action=2;break;case 'e':case 'E':action=3;break;case 'o':case 'O':action=4;break;case 'm':case 'M':action=5;break;case 't':case 'T':action=6;break;case 'q':case 'Q':quit=true;break;}}
    else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){if(y>=78 && y<120){if(x>=45 && x<265)action=1;else if(x>=277 && x<507)action=2;}else if(x>=45 && x<507){if(y>=140 && y<182)action=3;else if(y>=192 && y<234)action=4;else if(y>=244 && y<286)action=5;else if(y>=296 && y<338)quit=true;else if(y>=348 && y<390)action=6;}}
    switch(action){case 1:expansion=false;draw(w);break;case 2:expansion=true;draw(w);break;case 3:if(expansion)choose_map(w);else open_map(w,"Explore.SC");break;case 4:choose_map(w);break;case 5:toggle_music(w);break;case 6:open_technologies(w);break;}
   }
  }
  if(text_pen_owned)ReleasePen(colors,(ULONG)text_pen);
  if(border_pen_owned)ReleasePen(colors,(ULONG)border_pen);
  CloseWindow(w);
  rc=0;
 }
 if(AslBase)CloseLibrary(AslBase);
 if(CyberGfxBase)CloseLibrary(CyberGfxBase);
 if(GfxBase)CloseLibrary((struct Library*)GfxBase);
 if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);
 if(audio_device)SDL_CloseAudioDevice(audio_device);
 if(audio_data)SDL_FreeWAV(audio_data);
 TTF_CloseFont(menu_font);TTF_CloseFont(detail_font);TTF_Quit();
 SDL_Quit();free(frame);printf("FRONTEND EXIT RC %d\n",rc);return rc;
}
