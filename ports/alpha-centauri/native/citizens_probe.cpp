/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "citizens_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
static alpha::Rules rules; static alpha::CitizenDefinitions definitions;
static bool emit(uint32_t n) { unsigned char b[4]; for (unsigned i=0;i<4;++i) { b[i]=(unsigned char)n;n>>=8; } return fwrite(b,1,4,stdout)==4; }
static int direct_controls() {
 using namespace alpha;
 static CitizenDefinitions before; static Rules saved_rules;
 memset(&definitions,0x5a,sizeof definitions);memcpy(&before,&definitions,sizeof before);
 const uint8_t* alias=(const uint8_t*)&definitions;
 if (read_citizen_definitions(alias,sizeof definitions,rules,Variant::base,definitions)!=Error::bad_row || memcmp(&before,&definitions,sizeof before)) return 21;
 memcpy(&saved_rules,&rules,sizeof rules);
 auto& overlapped=*reinterpret_cast<CitizenDefinitions*>(&rules);
 if (read_citizen_definitions(nullptr,0,rules,Variant::base,overlapped)!=Error::bad_row || memcmp(&saved_rules,&rules,sizeof rules)) return 21;
 if (read_citizen_definitions((const uint8_t*)(UINTPTR_MAX-1),4,rules,Variant::base,definitions)!=Error::bad_row || memcmp(&before,&definitions,sizeof before)) return 21;
 if (read_citizen_definitions(nullptr,0,rules,Variant::base,definitions)!=Error::missing_section || definitions.count) return 21;
 const uint8_t data[]="#CITIZENS";
 if (read_citizen_definitions(data,sizeof data-1,rules,Variant::base,definitions)!=Error::wrong_count || definitions.count) return 21;
 rules.count=technology_count;
 if (read_citizen_definitions(data,sizeof data-1,rules,(Variant)99,definitions)!=Error::bad_row || definitions.count) return 21;
 if (read_citizen_definitions(data,sizeof data-1,rules,Variant::base,definitions)!=Error::wrong_count || definitions.count) return 21;
 puts("7 direct citizen controls passed");return 0;
}
int main(int argc,char** argv) {
 if (argc==2 && !strcmp(argv[1],"--selftest")) return direct_controls();
 if (argc!=3 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))) return 20;
 FILE* f=fopen(argv[2],"rb"); if (!f) return 20;
 if (fseek(f,0,SEEK_END)) { fclose(f);return 20; }
 const long size=ftell(f);rewind(f);if (size<=0 || size>1048576) { fclose(f);return 20; }
 auto* bytes=(uint8_t*)malloc((size_t)size);if (!bytes) { fclose(f);return 20; }
 bool read=fread(bytes,1,(size_t)size,f)==(size_t)size;fclose(f);if (!read) { free(bytes);return 20; }
 auto error=alpha::read_technologies(bytes,(size_t)size,rules);
 if (error==alpha::Error::ok) error=alpha::read_citizen_definitions(bytes,(size_t)size,rules,!strcmp(argv[1],"base")?alpha::Variant::base:alpha::Variant::alien_crossfire,definitions);
 free(bytes);
 if (error!=alpha::Error::ok) { if (definitions.count) return 21;fprintf(stderr,"ERROR %s\n",alpha::error_name(error));return 20; }
 if (!emit((uint32_t)definitions.count) || !emit((uint32_t)definitions.variant)) return 21;
 for (const auto& x:definitions.rows) {
  if (fwrite(x.singular,1,128,stdout)!=128 || fwrite(x.plural,1,128,stdout)!=128) return 21;
  for (auto value:{x.prerequisite,x.obsolete,x.economy,x.psych,x.labs}) if (!emit((uint32_t)value)) return 21;
 }
 return 0;
}
