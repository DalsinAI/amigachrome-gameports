/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_fixed_transform.h"
#include <stdio.h>
#include <string.h>
int main(){const float identity[]={1,0,0,0,1,0,0,0,1},origin[]={0,0,0};alpha::CvrFixedBasis basis{};if(!alpha::prepare_cvr_fixed_basis(identity,basis))return 20;
 const int wide[]={32768,-32769,65534,-65536};for(int v:wide){const int32_t group[]={v,0,0};const int16_t point[]={123,0,0};alpha::CvrFixedSeed seed{};if(!alpha::prepare_cvr_fixed_group_seed(origin,identity,group,seed)||seed.local[0]!=v)return 20;int32_t x,y;uint32_t z;alpha::cvr_fixed_position(basis,seed,point,x,y,z);if(x!=123*256||y||z)return 20;}
 const float fractional[]={.49f/256,0,0,0,1,0,0,0,1},offset[]={.49f/256,0,0};const int16_t point[]={0,0,1};alpha::CvrFixedSeed seeds[2]{};if(!alpha::prepare_cvr_fixed_basis(fractional,basis))return 20;
 for(int32_t i=0;i<2;++i){const int32_t group[]={i,0,0};if(!alpha::prepare_cvr_fixed_group_seed(offset,fractional,group,seeds[i]))return 20;int32_t x,y;uint32_t z;alpha::cvr_fixed_position(basis,seeds[i],point,x,y,z);if(x!=i||y||z!=65536)return 20;}
 const int32_t bad[]={65535,-65537};for(int32_t v:bad){auto before=seeds[0];const int32_t group[]={v,0,0};if(alpha::prepare_cvr_fixed_group_seed(origin,identity,group,seeds[0])||memcmp(&before,&seeds[0],sizeof(before)))return 20;}
 puts("CVR GROUP FIXED 4widened+2same-position-distinct-seed+2invalid controls PASS");return 0;}
