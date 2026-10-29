/* SPDX-License-Identifier: GPL-3.0-or-later. See base_commerce.h. */
#include "base_commerce.h"
namespace alpha {namespace {
bool overlap(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
}
CommerceError commerce_partner(const CommercePartnerFacts&f,CommercePartnerResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return CommerceError::bad_buffer;
 if(f.fresh_our_intake<0||f.fresh_their_intake<0||f.technology_count<0||f.technology_count>89||f.our_bonus< -1||f.their_bonus< -1)return CommerceError::bad_state;
 CommercePartnerResult r{};int32_t total;
 if(!put((int64_t)f.fresh_our_intake+f.fresh_their_intake,total)||!put((int64_t)total+7,total))return CommerceError::overflow;
 r.base_value=total/8;if(f.global_trade_pact&&!put(2LL*r.base_value,r.base_value))return CommerceError::overflow;
 const int32_t denominator=f.technology_count+1;int32_t factor,product;
 if(!put(1LL+f.our_bonus,factor)||!put((int64_t)r.base_value*factor,product)||!put((int64_t)denominator/2+product,product))return CommerceError::overflow;
 r.import_value=product/denominator;
 if(!put(1LL+f.their_bonus,factor)||!put((int64_t)r.base_value*factor,product)||!put((int64_t)denominator/2+product,product))return CommerceError::overflow;
 r.export_value=product/denominator;
 if(!f.pact){r.import_value/=2;r.export_value/=2;}
 if(f.our_governor&&!put(1LL+r.import_value,r.import_value))return CommerceError::overflow;
 if(f.their_governor&&!put(1LL+r.export_value,r.export_value))return CommerceError::overflow;
 out=r;return CommerceError::ok;
}
}
