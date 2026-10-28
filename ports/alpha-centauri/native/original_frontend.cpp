/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original-asset title/menu and audio front end for the restoration preview.
 */
#define SDL_MAIN_HANDLED
#include "pcx_image.h"
#include "technology_browser.h"
#include "play_session.h"
#include "ui_newgame.h"
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
#include <time.h>
extern struct IntuitionBase* IntuitionBase;extern struct GfxBase* GfxBase;extern struct Library* CyberGfxBase;
struct Library* AslBase;
int alpha_original_view(int argc,char** argv);
static alpha::PcxImage base_title,expansion_title;
static uint8_t* frame;static bool expansion=false,expansion_available=false,tutorial_available=false,music_on=true;static SDL_AudioDeviceID audio_device;
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
 button(w,45,78,220,"Alpha Centauri [A]",!expansion);button(w,277,78,230,expansion_available?"Alien Crossfire [X]":"Crossfire unavailable",expansion);
 button(w,45,140,462,expansion?"Choose expansion scenario [E]":tutorial_available?"Explore original tutorial [E]":"Choose original scenario [E]");
 button(w,45,192,462,"Open original scenario or save... [O]");
 button(w,45,244,462,audio_device?(music_on?"Original menu music: ON [M]":"Original menu music: OFF [M]"):"Original menu music unavailable");
 button(w,45,296,462,"Exit [Q]");
 button(w,45,348,462,"Technology discovery tree [T]");
 button(w,45,600,462,"New game [N]");button(w,45,652,462,"Play a saved game [P]");
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 if(!expansion){button(w,45,490,600,"Local actions: open world / checkpoint [L]");text(w->RPort,58,560,"Limited movement and facility orders; no End Turn.",true);}
#endif
 FillPixelArray(w->RPort,45,410,600,64,0x00100905u);SetAPen(w->RPort,(ULONG)text_pen);SetDrMd(w->RPort,JAM1);text(w->RPort,58,434,status,true);
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 text(w->RPort,58,457,"Explore is read-only; L opens limited local actions.",true);
#else
 text(w->RPort,58,457,"N starts a new game, P plays on from a save; the others are read-only.",true);
#endif
 printf("FRONTEND variant=%s music=%d device=%lu\n",expansion?"expansion":"base",music_on,(unsigned long)audio_device);fflush(stdout);
}
enum MapMode { MAP_VIEW=0,MAP_LOCAL=1,MAP_PLAY=2,MAP_PLAY_CURRENT=3 };
static void open_map(struct Window* w,const char* path,int mode=MAP_VIEW){
 static const char* const flags[4]={"","--local","--play","--play-current"};
 char* args[7]={const_cast<char*>("alpha-original-view"),const_cast<char*>(expansion?"expansion":"base"),const_cast<char*>(expansion?"alphax.txt":"alpha.txt"),const_cast<char*>(path),const_cast<char*>("texture.pcx"),const_cast<char*>("Units.pcx"),const_cast<char*>(flags[mode&3])};
 const bool local=mode!=MAP_VIEW;
 // The opening-menu loop belongs to this screen. Do not play it as a
 // substitute for the world's not-yet-restored ambient music system.
 release_menu_audio();
 int rc=alpha_original_view(local?7:6,args);
 restore_menu_audio();
 snprintf(status,sizeof(status),rc==0?"Returned from the original world view.":"Could not open that game/rules profile or a required original asset.");printf("FRONTEND MAP RC %d music_resumed=%d\n",rc,audio_device && music_on);fflush(stdout);ActivateWindow(w);draw(w);
}
static void choose_map(struct Window* w,int mode=MAP_VIEW){
 if(!AslBase){snprintf(status,sizeof(status),"The file requester library is unavailable.");draw(w);return;}
 auto* req=(struct FileRequester*)AllocAslRequestTags(ASL_FileRequest,ASLFR_Window,(ULONG)w,ASLFR_TitleText,(ULONG)"Open an original Alpha scenario or save",ASLFR_InitialDrawer,(ULONG)"PROGDIR:",TAG_DONE);
 if(!req){snprintf(status,sizeof(status),"Could not open the file requester.");draw(w);return;}
 char path[1024];bool valid=false;
 if(AslRequestTags(req,TAG_DONE)){
  size_t n=strlen(req->fr_Drawer);int used=snprintf(path,sizeof(path),"%s%s%s",req->fr_Drawer,n && req->fr_Drawer[n-1]!=':' && req->fr_Drawer[n-1]!='/'?"/":"",req->fr_File);valid=used>0 && (size_t)used<sizeof(path);
 }
 FreeAslRequest(req);ActivateWindow(w);if(valid)open_map(w,path,mode);else draw(w);
}
static void open_technologies(struct Window* w){
 release_menu_audio();
 int rc=alpha_technology_browser(expansion?"alphax.txt":"alpha.txt");
 restore_menu_audio();
 snprintf(status,sizeof(status),rc==0?"Returned from technology discovery tree.":"Technology rules/browser could not be opened.");
 printf("FRONTEND TECHNOLOGY RC %d music_resumed=%d\n",rc,audio_device && music_on);fflush(stdout);ActivateWindow(w);draw(w);
}
/* New game: WP7's ui_newgame screens, drawn with this screen's fonts. */
static void draw_new_game(struct Window* w,const alpha::ui::NewGameState& state){
 const auto view=alpha::ui::newgame_view(state);
 FillPixelArray(w->RPort,45,70,700,700,0x00100905u);SetAPen(w->RPort,(ULONG)text_pen);SetDrMd(w->RPort,JAM1);
 text(w->RPort,70,112,view.title.c_str());
 int y=150;
 for(const auto& line:view.lines){if(y>740)break;
  if(line.selected)FillPixelArray(w->RPort,60,(UWORD)(y-20),670,26,0x003b2c17u);
  text(w->RPort,line.heading?70:90,y,line.text.c_str(),!line.heading);y+=line.heading?32:27;}
 text(w->RPort,70,762,"Cursor keys or 8/2/4/6 choose, RETURN accepts, ESC goes back.",true);
}
static void new_game(struct Window* w){
 auto& game=alpha::play::session();
 if(!alpha::play::Session::new_game_available()){snprintf(status,sizeof(status),"New games are not in this build.");draw(w);return;}
 if(game.load_rules("",expansion?1:0)!=alpha::play::PLAY_OK){snprintf(status,sizeof(status),"Rules: %.140s",game.rules_error());draw(w);return;}
 game.bind();
 alpha::ui::NewGameState state;alpha::ui::newgame_open(state,(uint32_t)time(nullptr));
 printf("FRONTEND NEWGAME open\n");fflush(stdout);
 draw_new_game(w,state);
 while(!alpha::ui::newgame_done(state)){
  WaitPort(w->UserPort);struct IntuiMessage* m;
  while((m=(struct IntuiMessage*)GetMsg(w->UserPort))){const ULONG kind=m->Class;const UWORD code=m->Code;ReplyMsg((struct Message*)m);int key=-1;
   if(kind==IDCMP_CLOSEWINDOW)key=alpha::ui::NGK_BACK;
   else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(w);draw_new_game(w,state);EndRefresh(w,TRUE);}
   else if(kind==IDCMP_RAWKEY)switch(code){case 0x4c:key=alpha::ui::NGK_UP;break;case 0x4d:key=alpha::ui::NGK_DOWN;break;case 0x4f:key=alpha::ui::NGK_LEFT;break;case 0x4e:key=alpha::ui::NGK_RIGHT;break;default:break;}
   else if(kind==IDCMP_VANILLAKEY)switch(code){case '8':case 'w':key=alpha::ui::NGK_UP;break;case '2':case 's':key=alpha::ui::NGK_DOWN;break;case '4':case 'a':key=alpha::ui::NGK_LEFT;break;case '6':case 'd':key=alpha::ui::NGK_RIGHT;break;
    case 13:case 10:key=alpha::ui::NGK_ENTER;break;case 27:case 8:key=alpha::ui::NGK_BACK;break;default:break;}
   if(key>=0 && !alpha::ui::newgame_done(state) && alpha::ui::newgame_key(state,(alpha::ui::NewGameKey)key)){
    printf("FRONTEND NEWGAME screen=%d row=%d start=%d\n",(int)state.screen,state.row,state.start_status);fflush(stdout);
    if(!alpha::ui::newgame_done(state))draw_new_game(w,state);}
  }
 }
 if(state.screen!=alpha::ui::NG_DONE){snprintf(status,sizeof(status),"New game cancelled.");draw(w);return;}
 const int r=game.adopt_new_game();
 printf("FRONTEND NEWGAME start=%s\n",alpha::play::play_status_name(r));fflush(stdout);
 if(r!=alpha::play::PLAY_OK){snprintf(status,sizeof(status),"The new game could not start: %s",alpha::play::play_status_name(r));draw(w);return;}
 open_map(w,"",MAP_PLAY_CURRENT);
}
static void toggle_music(struct Window* w){if(audio_device){music_on=!music_on;SDL_PauseAudioDevice(audio_device,music_on?0:1);}draw(w);}
int main(int argc,char**){
 if(argc!=1){puts("Run alpha-restoration from the directory containing original game assets.");return 20;}
 if(!image("openinga.pcx",base_title)){puts("Original Alpha Centauri title PCX is required: openinga.pcx");return 20;}
 expansion_available=image("xopeninga.pcx",expansion_title);
 FILE* tutorial=fopen("Explore.SC","rb");tutorial_available=tutorial!=nullptr;if(tutorial)fclose(tutorial);
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
 if(IntuitionBase && GfxBase && CyberGfxBase)w=OpenWindowTags(nullptr,WA_Title,(ULONG)"Alpha Centauri / Alien Crossfire - restoration preview",WA_Width,1044,WA_Height,800,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
 if(!w)fprintf(stderr,"Cannot open menu window (1044x800): intuition=%d graphics=%d cybergraphics=%d\n",IntuitionBase!=nullptr,GfxBase!=nullptr,CyberGfxBase!=nullptr);
 if(w){
  auto* colors=w->WScreen->ViewPort.ColorMap;text_pen=ObtainBestPen(colors,0xeeeeeeee,0xdddddddd,0xaaaaaaaa,TAG_DONE);border_pen=ObtainBestPen(colors,0xaaaaaaaau,0x88888888u,0x44444444u,TAG_DONE);text_pen_owned=text_pen>=0;border_pen_owned=border_pen>=0;if(text_pen<0)text_pen=1;if(border_pen<0)border_pen=1;
  draw(w);bool quit=false;
  while(!quit){WaitPort(w->UserPort);struct IntuiMessage* m;
   while((m=(struct IntuiMessage*)GetMsg(w->UserPort))){ULONG kind=m->Class;UWORD code=m->Code;WORD x=m->MouseX,y=m->MouseY;ReplyMsg((struct Message*)m);int action=0;
    if(kind==IDCMP_CLOSEWINDOW)quit=true;
    else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(w);draw(w);EndRefresh(w,TRUE);}
    else if(kind==IDCMP_VANILLAKEY){switch(code){case 'a':case 'A':action=1;break;case 'x':case 'X':action=2;break;case 'e':case 'E':action=3;break;case 'o':case 'O':action=4;break;case 'm':case 'M':action=5;break;case 't':case 'T':action=6;break;case 'n':case 'N':action=8;break;case 'p':case 'P':action=9;break;case 'q':case 'Q':quit=true;break;
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    case 'l':case 'L':if(!expansion)action=7;break;
#endif
    }}
    else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){if(y>=78 && y<120){if(x>=45 && x<265)action=1;else if(x>=277 && x<507)action=2;}else if(x>=45 && x<507){if(y>=140 && y<182)action=3;else if(y>=192 && y<234)action=4;else if(y>=244 && y<286)action=5;else if(y>=296 && y<338)quit=true;else if(y>=348 && y<390)action=6;else if(y>=600 && y<642)action=8;else if(y>=652 && y<694)action=9;}}
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN && !expansion && x>=45 && x<645 && y>=490 && y<532)action=7;
#endif
    switch(action){case 1:expansion=false;draw(w);break;case 2:if(expansion_available)expansion=true;else snprintf(status,sizeof(status),"Original Alien Crossfire title art is unavailable: xopeninga.pcx");draw(w);break;case 3:if(expansion || !tutorial_available)choose_map(w);else open_map(w,"Explore.SC");break;case 4:choose_map(w);break;case 5:toggle_music(w);break;case 6:open_technologies(w);break;case 8:new_game(w);break;case 9:choose_map(w,MAP_PLAY);break;
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    case 7:choose_map(w,MAP_LOCAL);break;
#endif
    }
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
