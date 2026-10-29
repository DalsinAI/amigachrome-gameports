/* SPDX-License-Identifier: GPL-3.0-or-later. */
#include "original_template.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static bool read32(FILE*f,int32_t&v){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;v=(int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);return true;}
static void put32(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 while(true){int32_t mode,language,size,alias;if(!read32(f,mode))break;if(!read32(f,language)||!read32(f,size)||!read32(f,alias))return 20;
  alpha::OriginalParseContext ctx;if(fread(ctx.strings,1,2560,f)!=2560)return 20;for(int i=0;i<10;++i)if(!read32(f,ctx.genders[i]))return 20;for(int i=0;i<10;++i)if(!read32(f,ctx.plurals[i]))return 20;if(!read32(f,ctx.default_gender)||!read32(f,ctx.default_plural))return 20;
  unsigned char text[8192];if(fread(text,1,sizeof text,f)!=sizeof text)return 20;const auto before=ctx;unsigned char text_before[8192];memcpy(text_before,text,sizeof text);alpha::OriginalTemplateInput in{(alpha::DiscoveryRuntimeObservation)mode,language,text,(size_t)size};const auto input_before=in;
  alpha::OriginalExpandedText out;memset(&out,0xa5,sizeof out);const auto prior=out;alpha::OriginalTemplateError error;
  if(alias==1){error=alpha::original_expand_template(ctx,in,*reinterpret_cast<alpha::OriginalExpandedText*>(&ctx));if(memcmp(&ctx,&before,sizeof ctx))return 21;}
  else if(alias==2){in.bytes=(const uint8_t*)&out;in.size=1;error=alpha::original_expand_template(ctx,in,out);}
  else if(alias==3){in.bytes=(const uint8_t*)(uintptr_t)(UINTPTR_MAX-3);in.size=8;error=alpha::original_expand_template(ctx,in,out);}
  else error=alpha::original_expand_template(ctx,in,out);
  if(memcmp(&ctx,&before,sizeof ctx)||memcmp(text,text_before,sizeof text))return 22;
  if(!alias&&memcmp(&in,&input_before,sizeof in))return 23;
  if(error!=alpha::OriginalTemplateError::ok&&memcmp(&out,&prior,sizeof out))return 24;
  put32((uint32_t)error);if(error==alpha::OriginalTemplateError::ok){put32((uint32_t)out.length);fwrite(out.bytes,1,out.length,stdout);}else put32(0);
 }
 fclose(f);return 0;}
