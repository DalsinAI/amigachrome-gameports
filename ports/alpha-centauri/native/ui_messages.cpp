/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_messages.h"
#include <stdio.h>
#include <string.h>
namespace alpha {
MessageLog::MessageLog(){clear();}
void MessageLog::clear(){memset(items_,0,sizeof(items_));head_=count_=offset_=0;total_=0;}
bool MessageLog::add(int32_t turn,MessageKind kind,const char* text){
 if(!text || !*text || (unsigned)kind>(unsigned)MessageKind::research)return false;
 char clean[sizeof(items_[0].text)];size_t n=0;
 for(;text[n] && n+1<sizeof(clean);++n){const unsigned char c=(unsigned char)text[n];clean[n]=(c<32 || c==127)?'?':(char)c;}
 clean[n]=0;
 ++total_;
 if(count_){LogMessage& last=items_[(head_+capacity-1)%capacity];
  if(last.turn==turn && last.kind==kind && !strcmp(last.text,clean)){if(last.repeat<65535)++last.repeat;return true;}}
 LogMessage& m=items_[head_];m.turn=turn;m.kind=kind;m.repeat=1;memcpy(m.text,clean,n+1);
 head_=(head_+1)%capacity;if(count_<capacity)++count_;
 if(offset_)++offset_; // keep the reader's scrolled view stable
 if(offset_>=count_)offset_=count_?count_-1:0;
 return true;
}
const LogMessage* MessageLog::newest(size_t age)const{return age<count_?&items_[(head_+capacity-1-age)%capacity]:nullptr;}
void MessageLog::scroll(int lines){
 long next=(long)offset_+lines;if(next<0)next=0;if(count_ && next>(long)count_-1)next=(long)count_-1;if(!count_)next=0;offset_=(size_t)next;
}
size_t layout_message_panel(const MessageLog& log,const UiFrame& f,const UiRect& r,int line_height,UiTextItem* items,size_t capacity){
 if(!ui_frame_valid(f) || r.w<40 || r.h<2*line_height || line_height<8 || line_height>64)return 0;
 static const uint8_t fill[3]={7,19,22},edge[3]={70,110,96},stripe[6][3]={{120,150,140},{90,200,120},{230,190,60},{220,70,60},{120,170,230},{190,130,230}};
 ui_panel(f,r,fill,edge);
 size_t n=0;char line[160];
 snprintf(line,sizeof(line),"MESSAGES (%lu)",(unsigned long)log.count());
 ui_text_item(items,capacity,n,r.x+8,r.y+line_height,r.w-16,3,true,line);
 const int rows=(r.h-line_height-6)/line_height;if(rows<=0)return n;
 const size_t start=log.offset();size_t shown=0;
 for(int row=0;row<rows;++row){const LogMessage* m=log.newest(start+(size_t)(rows-1-row));if(!m)continue;
  const int baseline=r.y+line_height*(row+2);
  ui_fill(f,{r.x+3,baseline-line_height+4,3,line_height-4},stripe[(unsigned)m->kind]);
  if(m->repeat>1)snprintf(line,sizeof(line),"T%ld  %s (x%u)",(long)m->turn,m->text,(unsigned)m->repeat);
  else snprintf(line,sizeof(line),"T%ld  %s",(long)m->turn,m->text);
  ui_text_item(items,capacity,n,r.x+10,baseline,r.w-24,m->kind==MessageKind::warning?5:0,false,line);++shown;}
 // Scroll indicator: one tick per hidden page on the right edge.
 if(log.count()>(size_t)rows){
  const int track=r.h-line_height-8,thumb=track*rows/(int)log.count()<4?4:track*rows/(int)log.count();
  const int pos=(int)((int64_t)(track-thumb)*(int64_t)(log.count()-rows-(start>log.count()-rows?log.count()-rows:start))/(int64_t)(log.count()-rows));
  ui_fill(f,{r.x+r.w-6,r.y+line_height+4,3,track},edge);ui_fill(f,{r.x+r.w-6,r.y+line_height+4+pos,3,thumb},stripe[0]);
 }
 (void)shown;return n;
}
}
