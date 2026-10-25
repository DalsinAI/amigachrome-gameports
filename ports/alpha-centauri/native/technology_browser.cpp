/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "technology_browser.h"
#include <string.h>
namespace alpha {
bool TechnologyTree::build(const Rules& rules){
 if(rules.count!=technology_count)return false;
 TechnologyTree result{};
 for(size_t i=0;i<technology_count;++i)result.depth[i]=-1;
 size_t active=0;
 for(size_t i=0;i<technology_count;++i){
  const auto& t=rules.technologies[i];
  for(int k=0;k<2;++k)if(t.prerequisites[k]<-2 || t.prerequisites[k]>=(int)technology_count)return false;
  if(t.prerequisites[0]!=-2 && t.prerequisites[1]!=-2)++active;
 }
 while(result.count<active){
  bool progress=false;
  for(size_t i=0;i<technology_count;++i){
   const auto& t=rules.technologies[i];
   if(result.depth[i]>=0 || t.prerequisites[0]==-2 || t.prerequisites[1]==-2)continue;
   int depth=0;bool ready=true;
   for(int k=0;k<2;++k)if(t.prerequisites[k]>=0){int p=t.prerequisites[k];if(result.depth[p]<0)ready=false;else if(depth<=result.depth[p])depth=result.depth[p]+1;}
   if(ready){result.depth[i]=(int16_t)depth;result.order[result.count++]=(uint16_t)i;progress=true;}
  }
  if(!progress)return false;
 }
 /* Stable level ordering retains the original ID ordering within each tier. */
 for(size_t i=1;i<result.count;++i){uint16_t n=result.order[i];size_t j=i;while(j && (result.depth[result.order[j-1]]>result.depth[n] || (result.depth[result.order[j-1]]==result.depth[n] && result.order[j-1]>n))){result.order[j]=result.order[j-1];--j;}result.order[j]=n;}
 *this=result;return true;
}
bool technology_description(const uint8_t* bytes,size_t length,int id,char* out,size_t capacity){
 if(!bytes || !out || capacity<1 || id<0 || id>999 || memchr(bytes,0,length))return false;
 char candidate[4096];size_t used=0;bool found=false;
 for(size_t pos=0;pos<length;){size_t start=pos;while(pos<length && bytes[pos]!='\n' && bytes[pos]!='\r')++pos;size_t n=pos-start;while(pos<length && (bytes[pos]=='\r'||bytes[pos]=='\n'))++pos;
  const uint8_t* line=bytes+start;
  if(n && line[0]=='#'){
   if(found)break;
   if(n>=6 && !memcmp(line,"#TECH",5)){int value=0;bool digits=true;for(size_t k=5;k<n;++k){if(line[k]<'0'||line[k]>'9'){digits=false;break;}value=value*10+line[k]-'0';if(value>999){digits=false;break;}}if(digits && value==id)found=true;}
   continue;
  }
  if(!found || !n || line[0]==';')continue;
  /* Braces/brackets are original hyperlink markup, not glyphs. */
  if(used && used+1<sizeof(candidate))candidate[used++]=' ';
  for(size_t k=0;k<n;++k){if(line[k]=='{'||line[k]=='}'||line[k]=='['||line[k]==']'||line[k]=='^')continue;if(used+1>=sizeof(candidate))return false;candidate[used++]=(char)line[k];}
 }
 if(!found || used+1>capacity)return false;
 candidate[used]=0;memcpy(out,candidate,used+1);return true;
}
}
