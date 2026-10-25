/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned technology browser: original parsed names/dependencies, no research mutation.
 */
#define SDL_MAIN_HANDLED
#include "technology_browser.h"
#include "original_ui_text.h"
#include "pcx_image.h"
#include <SDL2/SDL.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern struct IntuitionBase* IntuitionBase;extern struct GfxBase* GfxBase;extern struct Library* CyberGfxBase;
namespace {
uint8_t* read(const char* path,size_t& size){size=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);if(n<1||n>2*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);p=nullptr;}else size=(size_t)n;fclose(f);return p;}
struct Speech {SDL_AudioDeviceID device=0;uint8_t* data=nullptr;Uint32 length=0,position=0;
 static void fill(void* state,Uint8* output,int length){auto& s=*(Speech*)state;memset(output,0,(size_t)length);Uint32 left=s.length-s.position,n=left<(Uint32)length?left:(Uint32)length;if(n){memcpy(output,s.data+s.position,n);s.position+=n;}}
 void stop(){if(device)SDL_CloseAudioDevice(device);device=0;if(data)SDL_FreeWAV(data);data=nullptr;length=position=0;}
 bool play(int id){stop();char name[80];snprintf(name,sizeof(name),"voices/tech%d.wav",id);SDL_AudioSpec spec{};if(!SDL_LoadWAV(name,&spec,&data,&length))return false;
  if(spec.format!=AUDIO_S16LSB || !length || length%2 || spec.channels!=1 || spec.freq!=22050){stop();return false;}
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  for(Uint32 i=0;i<length;i+=2){Uint8 n=data[i];data[i]=data[i+1];data[i+1]=n;}
#endif
  spec.format=AUDIO_S16SYS;spec.samples=1024;spec.callback=fill;spec.userdata=this;device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);if(!device){stop();return false;}SDL_PauseAudioDevice(device,0);return true;
 }
 ~Speech(){stop();}
};
void paragraph(alpha::OriginalUiText& text,Window* w,int x,int y,const char* source){
 char line[88];size_t pos=0;for(int row=0;source[pos] && row<13;++row){size_t count=0;while(source[pos+count] && count<76)++count;if(source[pos+count]){size_t cut=count;while(cut && source[pos+cut]!=' ')--cut;if(cut)count=cut;}memcpy(line,source+pos,count);line[count]=0;text.draw(w->RPort,x,y+row*19,line,660);pos+=count;while(source[pos]==' ')++pos;}
}
}
int alpha_technology_browser(const char* rules_path){
 size_t n=0;uint8_t* bytes=read(rules_path,n);auto* rules_storage=(alpha::Rules*)calloc(1,sizeof(alpha::Rules));alpha::TechnologyTree tree{};if(!bytes || !rules_storage){free(bytes);free(rules_storage);return 20;}auto& rules=*rules_storage;
 bool valid=alpha::read_technologies(bytes,n,rules)==alpha::Error::ok && tree.build(rules);free(bytes);if(!valid || !tree.count){free(rules_storage);return 20;}
 size_t descriptions_size=0;uint8_t* descriptions=read("techlongs.txt",descriptions_size);
 alpha::OriginalUiText font;if(!font.open()){free(descriptions);free(rules_storage);return 20;}
 Window* w=OpenWindowTags(nullptr,WA_Title,(ULONG)"Alpha Centauri - technology dependencies",WA_Width,1000,WA_Height,720,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
 if(!w){free(descriptions);free(rules_storage);return 20;}
 Speech speech;int selected=tree.order[0],top=0;bool quit=false;char status[100]="Read-only rules browser: no research or unlock state is changed.";
 auto draw=[&](){FillPixelArray(w->RPort,8,22,984,690,0x00040709);font.draw(w->RPort,24,51,"TECHNOLOGY / DISCOVERY",920,true);font.draw(w->RPort,24,76,"N/P: browse  1/2: prerequisites  D: first dependent  V: voice  Q: return",950);
  for(int row=0;row<27 && top+row<(int)tree.count;++row){int id=tree.order[top+row];char name[180];snprintf(name,sizeof(name),"%2d  %s",tree.depth[id],rules.technologies[id].name);if(id==selected)FillPixelArray(w->RPort,20,92+row*20,290,20,0x00283140);font.draw(w->RPort,26,109+row*20,name,280);}
  char icon_path[80];snprintf(icon_path,sizeof(icon_path),"techs/tech%03d.pcx",selected);size_t icon_size=0;uint8_t* icon_bytes=read(icon_path,icon_size);alpha::PcxImage icon;
  if(icon_bytes && icon.load(icon_bytes,icon_size) && icon.width()==215 && icon.height()==215){auto* rgb=(uint8_t*)malloc(215u*215u*3u);if(rgb){for(unsigned y=0;y<215;++y)for(unsigned x=0;x<215;++x){const uint8_t* src=icon.rgb(x,y);auto* dst=rgb+(y*215+x)*3;if(icon.pixels()[y*215+x]==255){dst[0]=4;dst[1]=7;dst[2]=9;}else memcpy(dst,src,3);}WritePixelArray(rgb,0,0,215*3,w->RPort,748,155,215,215,RECTFMT_RGB);free(rgb);}}free(icon_bytes);
  auto& t=rules.technologies[selected];font.draw(w->RPort,334,116,t.name,640,true,alpha::UiInk::title);char line[180];snprintf(line,sizeof(line),"ID %d / %s / prerequisite tier %d",selected,t.key,tree.depth[selected]);font.draw(w->RPort,334,143,line,640);
  for(int k=0;k<2;++k){int p=t.prerequisites[k];snprintf(line,sizeof(line),"[%d] %s",k+1,p>=0?rules.technologies[p].name:"No prerequisite");font.draw(w->RPort,334,177+k*24,line,400);}
  font.draw(w->RPort,334,244,"LEADS TO",640,true);int row=0;for(size_t i=0;i<rules.count && row<7;++i)if(tree.depth[i]>=0 && (rules.technologies[i].prerequisites[0]==selected || rules.technologies[i].prerequisites[1]==selected)){font.draw(w->RPort,334,268+row*20,rules.technologies[i].name,400);++row;}
  char description[4096];if(descriptions && alpha::technology_description(descriptions,descriptions_size,selected,description,sizeof(description)))paragraph(font,w,334,438,description);else font.draw(w->RPort,334,438,"Original technology description unavailable.",640);
  font.draw(w->RPort,24,694,status,950);printf("TECHNOLOGY SELECT %d key=%s tier=%d active=%lu\n",selected,t.key,tree.depth[selected],(unsigned long)tree.count);fflush(stdout);
 };
 draw();while(!quit){WaitPort(w->UserPort);IntuiMessage* msg;while((msg=(IntuiMessage*)GetMsg(w->UserPort))){ULONG kind=msg->Class;UWORD code=msg->Code;int x=msg->MouseX,y=msg->MouseY;ReplyMsg((Message*)msg);int old=selected;
  if(kind==IDCMP_CLOSEWINDOW)quit=true;
  else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(w);draw();EndRefresh(w,TRUE);}
  else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN && x>=20 && x<310 && y>=92 && y<632){int index=top+(y-92)/20;if(index<(int)tree.count)selected=tree.order[index];}
  else if(kind==IDCMP_VANILLAKEY){if(code=='q'||code=='Q')quit=true;else if(code=='v'||code=='V'){bool played=speech.play(selected);snprintf(status,sizeof(status),played?"Original technology voice playing (local decoded original recording).":"Voice unavailable: requires voices/tech<ID>.wav PCM16 mono 22050.");printf("TECHNOLOGY VOICE %d %d error=%s\n",selected,played,played?"none":SDL_GetError());draw();}else if(code=='1'||code=='2'){int p=rules.technologies[selected].prerequisites[code-'1'];if(p>=0)selected=p;}else if(code=='d'||code=='D'){for(size_t i=0;i<tree.count;++i){int id=tree.order[i];if(rules.technologies[id].prerequisites[0]==selected||rules.technologies[id].prerequisites[1]==selected){selected=id;break;}}}else if(code=='n'||code=='N'||code=='p'||code=='P'){for(size_t i=0;i<tree.count;++i)if(tree.order[i]==selected){selected=tree.order[(i+tree.count+((code=='p'||code=='P')?-1:1))%tree.count];break;}}}
  if(old!=selected){speech.stop();for(size_t i=0;i<tree.count;++i)if(tree.order[i]==selected){if((int)i<top)top=(int)i;else if((int)i>=top+27)top=(int)i-26;}draw();}
 }}speech.stop();CloseWindow(w);free(descriptions);free(rules_storage);puts("TECHNOLOGY EXIT RC 0");return 0;
}
