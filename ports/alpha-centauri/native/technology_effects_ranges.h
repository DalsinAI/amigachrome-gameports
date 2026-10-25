/* SPDX-License-Identifier: GPL-3.0-or-later. Internal range validation. */
#ifndef ALPHA_TECHNOLOGY_EFFECTS_RANGES_H
#define ALPHA_TECHNOLOGY_EFFECTS_RANGES_H
#include <stdint.h>
#include <stddef.h>
namespace alpha { namespace detail {
inline bool effects_range_overlap(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(n>UINTPTR_MAX-x || m>UINTPTR_MAX-y)return true;
 return x<y+m && y<x+n;
}
inline bool effects_array_overlap(const void* a,size_t n,const void* b,size_t count,size_t element){
 if(count && element>SIZE_MAX/count)return true;
 return effects_range_overlap(a,n,b,count*element);
}
}}
#endif
