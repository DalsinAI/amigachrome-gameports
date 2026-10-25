/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_ui_text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <exec/types.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <stdlib.h>
namespace alpha {
OriginalUiText::OriginalUiText():body_(nullptr),heading_(nullptr),initialized_(false){}
OriginalUiText::~OriginalUiText(){close();}
bool OriginalUiText::open(){
 close();if(TTF_Init()!=0)return false;initialized_=true;
 body_=TTF_OpenFont("arialn.ttf",17);heading_=TTF_OpenFont("ALPHC___.TTF",18);
 if(!body_ || !heading_){close();return false;}return true;
}
void OriginalUiText::close(){if(body_)TTF_CloseFont(body_);if(heading_)TTF_CloseFont(heading_);body_=heading_=nullptr;if(initialized_)TTF_Quit();initialized_=false;}
bool OriginalUiText::draw(RastPort* rp,int x,int baseline,const char* message,int max_width,bool heading){
 if(!body_ || !heading_ || !rp || !message || x<0 || baseline<0 || max_width<=0 || max_width>2048)return false;
 SDL_Color ink{220,228,207,255};SDL_Surface* text=TTF_RenderUTF8_Blended(heading?heading_:body_,message,ink);if(!text)return false;
 SDL_Surface* rgba=SDL_ConvertSurfaceFormat(text,SDL_PIXELFORMAT_RGBA32,0);SDL_FreeSurface(text);if(!rgba)return false;
 int width=rgba->w;if(width>max_width)width=max_width;int height=rgba->h,top=baseline-height;
 if(width<=0 || height<=0 || height>64 || top<0){SDL_FreeSurface(rgba);return false;}
 auto* pixels=(uint8_t*)malloc((size_t)width*height*3);if(!pixels){SDL_FreeSurface(rgba);return false;}
 ReadPixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);
 for(int row=0;row<height;++row)for(int col=0;col<width;++col){const auto* src=(const uint8_t*)rgba->pixels+(size_t)row*rgba->pitch+col*4;auto* dst=pixels+((size_t)row*width+col)*3;unsigned a=src[3];for(int c=0;c<3;++c)dst[c]=(uint8_t)((src[c]*a+dst[c]*(255-a)+127)/255);}
 WritePixelArray(pixels,0,0,width*3,rp,x,top,width,height,RECTFMT_RGB);free(pixels);SDL_FreeSurface(rgba);return true;
}
}
