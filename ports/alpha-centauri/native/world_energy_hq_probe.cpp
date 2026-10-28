/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only entry for exact private scan.
 * Function sections discard the complete composer; no replacement scan or
 * fabricated dependency callback is linked. Public whole-composer tests cover
 * its call site separately. */
#include "world_energy.cpp"
#include "local_actions_test_state.h"
#include <stdio.h>
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}
 auto*b=(uint8_t*)malloc((size_t)n);if(!b){fclose(f);return 20;}bool ok=fread(b,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(b);return 20;}
 alpha::OriginalWorld w,baseline;if(w.load(b,(size_t)n,123)!=alpha::WorldLoadError::ok||baseline.load(b,(size_t)n,123)!=alpha::WorldLoadError::ok||!w.base(0)){free(b);return 20;}
 const uint32_t distance=(uint32_t)alpha::headquarters(w,*w.base(0));uint8_t out[4];for(unsigned i=0;i<4;++i)out[i]=(uint8_t)(distance>>(i*8));
 ok=fwrite(out,1,4,stdout)==4&&alpha_test::same_world(w,baseline)&&w.runtime_seed()==123;free(b);return ok?0:21;
}
