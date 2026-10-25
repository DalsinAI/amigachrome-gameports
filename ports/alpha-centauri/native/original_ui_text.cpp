/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_ui_text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <exec/types.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <stdlib.h>
#include <stdio.h>
namespace alpha {
OriginalUiText::OriginalUiText():body_(nullptr),heading_(nullptr),initialized_(false){}
OriginalUiText::~OriginalUiText(){close();}
bool OriginalUiText::open(){
 close();if(TTF_Init()!=0)return false;initialized_=true;
 // The original status/datalink screens request Arial, not Arial Narrow or
 // the title typeface. Use owner-provided system fonts when available. Their
 // Win32 logical-height/hinting metrics are not identical to SDL_ttf pixels.
 body_=TTF_OpenFont("Arial.ttf",12);
 if(body_){
  heading_=TTF_OpenFont("Arial_Bold.ttf",14);
  if(!heading_){heading_=TTF_OpenFont("Arial.ttf",14);if(heading_)TTF_SetFontStyle(heading_,TTF_STYLE_BOLD);}
 }
 if(!body_ || !heading_){
  if(body_)TTF_CloseFont(body_);
  if(heading_)TTF_CloseFont(heading_);
  body_=TTF_OpenFont("arialn.ttf",13);heading_=TTF_OpenFont("ALPHC___.TTF",16);
  printf("UI FONT fallback=Arial-Narrow13/title16; original Arial unavailable\n");
 }else printf("UI FONT face=Arial12 heading=Arial-Bold14; Win32 metrics not identical\n");
 if(!body_ || !heading_){close();return false;}return true;
}
void OriginalUiText::close(){if(body_)TTF_CloseFont(body_);if(heading_)TTF_CloseFont(heading_);body_=heading_=nullptr;if(initialized_)TTF_Quit();initialized_=false;}
bool OriginalUiText::draw(RastPort* rp,int x,int baseline,const char* message,int max_width,bool heading,UiInk role){
 if(!body_ || !heading_ || !rp || !message || x<0 || baseline<0 || max_width<=0 || max_width>2048)return false;
 // Compact, crisp text and role colours follow the original UI references.
 // This is a presentation profile, not a claim of recovered original metrics.
 SDL_Color ink{150,172,153,255};
 switch(role){
 case UiInk::label:ink={128,148,195,255};break;
 case UiInk::value:ink={184,194,88,255};break;
 case UiInk::title:ink={216,205,92,255};break;
 case UiInk::map_label:ink={232,224,112,255};break;
 case UiInk::warning:ink={205,164,90,255};break;
 case UiInk::body:break;
 }
 SDL_Surface* text=TTF_RenderUTF8_Solid(heading?heading_:body_,message,ink);if(!text)return false;
 SDL_Surface* rgba=SDL_ConvertSurfaceFormat(text,SDL_PIXELFORMAT_RGBA32,0);SDL_FreeSurface(text);if(!rgba)return false;
 int width=rgba->w;if(width>max_width)width=max_width;int height=rgba->h,top=baseline-TTF_FontAscent(heading?heading_:body_);
 if(width<=0 || height<=0 || height>64 || top<0){SDL_FreeSurface(rgba);return false;}
 auto* pixels=(uint8_t*)malloc((size_t)width*height*3);if(!pixels){SDL_FreeSurface(rgba);return false;}
 ReadPixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);
 for(int row=0;row<height;++row)for(int col=0;col<width;++col){const auto* src=(const uint8_t*)rgba->pixels+(size_t)row*rgba->pitch+col*4;auto* dst=pixels+((size_t)row*width+col)*3;unsigned a=src[3];for(int c=0;c<3;++c)dst[c]=(uint8_t)((src[c]*a+dst[c]*(255-a)+127)/255);}
 WritePixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);free(pixels);SDL_FreeSurface(rgba);return true;
}
}
