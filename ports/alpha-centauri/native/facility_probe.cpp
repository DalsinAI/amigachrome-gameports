/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "rules_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules technology;static alpha::FacilityDefinitions definitions;
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){
 if(argc!=3 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 FILE* f=fopen(argv[2],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}const long n=ftell(f);rewind(f);if(n<=0 || n>1048576){fclose(f);return 20;}
 auto* bytes=(uint8_t*)malloc((size_t)n);if(!bytes){fclose(f);return 20;}const bool read=fread(bytes,1,(size_t)n,f)==(size_t)n;fclose(f);if(!read){free(bytes);return 20;}
 auto error=alpha::read_technologies(bytes,(size_t)n,technology);
 if(error==alpha::Error::ok){definitions.slot_count=123;error=alpha::read_facility_definitions(bytes,(size_t)n,technology,!strcmp(argv[1],"base")?alpha::Variant::base:alpha::Variant::alien_crossfire,definitions);}
 free(bytes);if(error!=alpha::Error::ok){if(definitions.slot_count)return 21;fprintf(stderr,"ERROR %s rc=20\n",alpha::error_name(error));return 20;}
 if(!emit((uint32_t)definitions.slot_count) || !emit((uint32_t)definitions.first_repeatable) || !emit((uint32_t)definitions.first_project) || !emit((uint32_t)definitions.project_count))return 21;
 for(size_t i=0;i<definitions.slot_count;++i){const auto& x=definitions.slots[i];if(fwrite(x.name,1,sizeof x.name,stdout)!=sizeof x.name || fwrite(x.effect,1,sizeof x.effect,stdout)!=sizeof x.effect || !emit((uint32_t)x.cost) || !emit((uint32_t)x.maintenance) || !emit((uint32_t)x.prerequisite) || !emit((uint32_t)x.declared_free_technology) || !emit((uint32_t)x.effective_free_technology))return 21;for(auto value:x.priorities)if(!emit((uint32_t)value))return 21;}
 return 0;
}
