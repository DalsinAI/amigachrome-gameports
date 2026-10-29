/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_panel.h"
#include <string.h>
namespace alpha {
bool ui_frame_valid(const UiFrame& f){return f.rgb && f.width && f.height && f.width<=4096 && f.height<=4096 && f.capacity>=(size_t)f.width*f.height*3;}
void ui_fill(const UiFrame& f,const UiRect& r,const uint8_t rgb[3]){
 if(!ui_frame_valid(f) || r.w<=0 || r.h<=0)return;
 const int x0=r.x<0?0:r.x,y0=r.y<0?0:r.y;
 const int x1=r.x+r.w>(int)f.width?(int)f.width:r.x+r.w,y1=r.y+r.h>(int)f.height?(int)f.height:r.y+r.h;
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){uint8_t* p=f.rgb+((size_t)y*f.width+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
}
void ui_panel(const UiFrame& f,const UiRect& r,const uint8_t fill[3],const uint8_t edge[3]){
 ui_fill(f,r,edge);ui_fill(f,{r.x+1,r.y+1,r.w-2,r.h-2},fill);
}
int ui_bar(const UiFrame& f,const UiRect& r,const int32_t values[3],const uint8_t colours[3][3],int32_t scale){
 if(scale<=0 || r.w<=0)return 0;
 int at=0;
 for(unsigned i=0;i<3;++i){if(values[i]<=0)continue;
  int64_t w=(int64_t)values[i]*r.w/scale;if(w<1)w=1;if(w>r.w-at)w=r.w-at;if(w<=0)break;
  ui_fill(f,{r.x+at,r.y,(int)w,r.h},colours[i]);at+=(int)w;}
 return at;
}
bool ui_text_item(UiTextItem* items,size_t capacity,size_t& count,int x,int baseline,int width,uint8_t ink,bool heading,const char* text){
 if(!items || count>=capacity || !text)return false;
 UiTextItem& t=items[count++];t.x=x;t.baseline=baseline;t.width=width;t.ink=ink;t.heading=heading;
 size_t n=0;for(;text[n] && n+1<sizeof(t.text);++n){const unsigned char c=(unsigned char)text[n];t.text[n]=(c<32 || c==127)?'?':(char)c;}
 t.text[n]=0;return true;
}
int ui_key_from_char(int c){
 switch(c){case '8':return UIK_UP;case '2':return UIK_DOWN;case '4':return UIK_LEFT;case '6':return UIK_RIGHT;case 13:case 10:return UIK_ENTER;case 27:return UIK_BACK;default:return UIK_NONE;}
}
void ui_list_step(int& cursor,int count,int delta){
 if(count<=0){cursor=0;return;}
 cursor=((cursor+delta)%count+count)%count;
}
}
