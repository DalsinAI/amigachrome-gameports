/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_social.h"
#include <stdio.h>
namespace alpha {
static const uint8_t fill[3]={7,19,22},edge[3]={92,128,112},cursor[3]={36,64,58},chosen[3]={20,52,30};
size_t render_social_panel(const SocialPanelModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 size_t n=0;if(!m.valid || !ui_frame_valid(f) || r.w<480 || r.h<240)return 0;
 ui_panel(f,r,fill,edge);char line[96];
 ui_text_item(items,capacity,n,r.x+16,r.y+28,r.w-32,3,true,"SOCIAL ENGINEERING");
 snprintf(line,sizeof(line),"Energy reserves %d",m.credits);ui_text_item(items,capacity,n,r.x+16,r.y+52,r.w-32,0,false,line);
 const int col_w=(r.w-140)/4;
 for(int c=0;c<4;++c){const int y=r.y+80+c*36;
  ui_text_item(items,capacity,n,r.x+16,y+16,110,1,false,m.categories[c]);
  for(int k=0;k<4;++k){const int x=r.x+130+k*col_w;
   if(c==m.row && k==m.col)ui_fill(f,{x-2,y,col_w-4,30},cursor);else if(m.current[c]==k)ui_fill(f,{x-2,y,col_w-4,30},chosen);
   ui_text_item(items,capacity,n,x+2,y+16,col_w-8,!m.available[c][k]?0:m.current[c]==k?2:1,false,m.models[c][k]);}}
 const int c=m.row,k=m.col;
 if(c>=0 && c<4 && k>=0 && k<4){
  if(!m.available[c][k])snprintf(line,sizeof(line),"%.23s is not available yet.",m.models[c][k]);
  else if(m.current[c]==k)snprintf(line,sizeof(line),"%.23s is the current %.23s choice.",m.models[c][k],m.categories[c]);
  else snprintf(line,sizeof(line),"Switch to %.23s: upheaval cost %d energy. ENTER applies.",m.models[c][k],m.cost[c][k]);
  ui_text_item(items,capacity,n,r.x+16,r.y+r.h-36,r.w-32,m.available[c][k] && m.cost[c][k]>m.credits?5:0,false,line);}
 ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,r.w-32,0,false,"8/2 category, 4/6 model, ENTER apply, ESC close");
 return n;
}
int social_panel_key(SocialPanelModel& m,int key){
 switch(key){
 case UIK_UP:ui_list_step(m.row,4,-1);return UIA_MOVED;
 case UIK_DOWN:ui_list_step(m.row,4,1);return UIA_MOVED;
 case UIK_LEFT:ui_list_step(m.col,4,-1);return UIA_MOVED;
 case UIK_RIGHT:ui_list_step(m.col,4,1);return UIA_MOVED;
 case UIK_ENTER:return m.row>=0 && m.row<4 && m.col>=0 && m.col<4 && m.available[m.row][m.col] && m.current[m.row]!=m.col?UIA_APPLY:UIA_NONE;
 case UIK_BACK:return UIA_CLOSE;
 default:return UIA_NONE;}
}
}
