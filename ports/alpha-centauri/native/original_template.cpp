/* SPDX-License-Identifier: GPL-3.0-or-later. See original_template.h. */
#include "original_template.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha { namespace {
bool digit(char c){return c>='0'&&c<='9';}
bool letter(char c){return(c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_';}
struct Sink {OriginalExpandedText result{};bool add(const char* p,size_t n){if(n>=sizeof result.bytes-result.length)return false;memcpy(result.bytes+result.length,p,n);result.length+=n;result.bytes[result.length]=0;return true;}};
bool slot(const OriginalParseContext& c,int i,const char*&p,size_t&n){if(i<0||i>9)return false;p=c.strings[i];const char* e=(const char*)memchr(p,0,256);if(!e)return false;n=(size_t)(e-p);return true;}
OriginalTemplateError selected(const OriginalParseContext& ctx,const char*p,const char*end,Sink&s){
 while(p<end&&*p==' ')++p;
 if(p==end)return OriginalTemplateError::bad_text;
 int32_t gender=0,plural=0;char selector=*p++;
 if(selector=='M'||selector=='m'||selector=='F'||selector=='f'||selector=='N'||selector=='n'){
  gender=selector=='M'||selector=='m'?0:selector=='F'||selector=='f'?1:2;
  if(p<end&&digit(*p))plural=*p++-'1';
 }else if(digit(selector)){const int i=selector-'0';gender=ctx.genders[i];plural=ctx.plurals[i];}
 else return OriginalTemplateError::unsupported;
 while(p<end&&*p==' ')++p;
 if(p<end&&*p==':')++p;
 const char* value=p;size_t size=(size_t)(end-p);unsigned count=0;
 for(const char*q=p;q<end;++q){if(*q==':')++count;if(*q=='<'||*q=='>')return OriginalTemplateError::unsupported;}
 if(!count){const char* q=p;while(q<end&&!digit(*q))++q;if(q<end){if(!slot(ctx,*q-'0',value,size))return OriginalTemplateError::bad_text;count=0;for(size_t i=0;i<size;++i)if(value[i]==':')++count;}}
 unsigned choice=0;
 if(count==1)choice=plural!=0;
 else if(count==2){if(gender<0||gender>2)return OriginalTemplateError::grammar_held;choice=(unsigned)gender;}
 else if(count==3)choice=(gender!=0)+2*(plural!=0);
 else if(count==5){if(gender<0||gender>2)return OriginalTemplateError::grammar_held;choice=(unsigned)gender+3*(plural!=0);}
 else choice=count;
 const char* finish=value+size;
 while(choice){const char* colon=(const char*)memchr(value,':',(size_t)(finish-value));if(!colon)return OriginalTemplateError::grammar_held;value=colon+1;--choice;}
 const char* stop=value;while(stop<finish&&*stop!=':'&&*stop!='>')++stop;
 return s.add(value,(size_t)(stop-value))?OriginalTemplateError::ok:OriginalTemplateError::overlong;
}
}
OriginalTemplateError original_expand_template(const OriginalParseContext& ctx,const OriginalTemplateInput& in,OriginalExpandedText& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&ctx,sizeof ctx)||effects_range_overlap(&out,sizeof out,&in,sizeof in)||(in.size&&(!in.bytes||effects_range_overlap(&out,sizeof out,in.bytes,in.size))))return OriginalTemplateError::bad_state;
 if(in.runtime!=DiscoveryRuntimeObservation::supplied)return OriginalTemplateError::runtime_required;
 if(in.language_mode==1)return OriginalTemplateError::unsupported;
 if(!in.bytes||!in.size)return OriginalTemplateError::bad_text;
 const size_t bound=in.size<8192?in.size:8192;const char* begin=(const char*)in.bytes;const char* end=(const char*)memchr(begin,0,bound);
 if(!end)return in.size>=8192?OriginalTemplateError::overlong:OriginalTemplateError::bad_text;
 Sink sink;const char*p=begin;
 while(p<end){const char* dollar=(const char*)memchr(p,'$',(size_t)(end-p));if(!dollar){if(!sink.add(p,(size_t)(end-p)))return OriginalTemplateError::overlong;break;}
  if(!sink.add(p,(size_t)(dollar-p)))return OriginalTemplateError::overlong;
  p=dollar+1;
  if(p==end)return OriginalTemplateError::unsupported;
  if(*p=='$'){if(!sink.add("$",1))return OriginalTemplateError::overlong;++p;continue;}
  if(*p=='<'){const char*close=(const char*)memchr(p+1,'>',(size_t)(end-p-1));if(!close)return OriginalTemplateError::bad_text;const auto e=selected(ctx,p+1,close,sink);if(e!=OriginalTemplateError::ok)return e;p=close+1;continue;}
  if((end-p>=6&&memcmp(p,"NUMBER",6)==0)||(end-p>=3&&(memcmp(p,"NUM",3)==0||memcmp(p,"HEX",3)==0))||(end-p>=5&&memcmp(p,"LINK<",5)==0))return OriginalTemplateError::unsupported;
  const char*q=p;while(q<end&&letter(*q))++q;if(q==p||q==end||!digit(*q))return OriginalTemplateError::unsupported;
  const char*value;size_t size;if(!slot(ctx,*q-'0',value,size))return OriginalTemplateError::bad_text;if(!sink.add(value,size))return OriginalTemplateError::overlong;p=q+1;
 }
 out=sink.result;return OriginalTemplateError::ok;
}
}
