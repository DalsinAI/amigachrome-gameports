/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "queue_shift.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(const unsigned char* p){uint32_t n=word(p);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char record[556];size_t count=0;
 for(;;){size_t n=fread(record,1,sizeof record,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof record || ++count>200000){fclose(f);return 20;}
 alpha::QueueShiftInput in{};in.queue_size=signed_word(record);for(unsigned i=0;i<10;++i)in.items[i]=signed_word(record+4+i*4);memcpy(in.units_queue,record+44,512);
 alpha::QueueShiftResult r{};r.queue_size=0x31415926;for(auto& v:r.items)v=0x31415926;memset(r.units_queue,0x5a,512);r.shifted=true;const auto error=alpha::queue_shift(in,r);
 if(error!=alpha::LogicError::ok){if(r.queue_size!=0x31415926 || !r.shifted)return 21;for(auto v:r.items)if(v!=0x31415926)return 21;for(auto v:r.units_queue)if(v!=0x5a)return 21;}
 if(!emit((uint32_t)error) || !emit((uint32_t)r.queue_size))return 20;
 for(auto v:r.items)if(!emit((uint32_t)v))return 20;
 if(fwrite(r.units_queue,1,512,stdout)!=512 || !emit(r.shifted))return 20;
 }
}
