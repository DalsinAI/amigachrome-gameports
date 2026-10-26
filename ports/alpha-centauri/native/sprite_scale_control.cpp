// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_scale.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <climits>
#include <initializer_list>
#include <new>
int main(){unsigned cases=0;alpha::SpriteScaleTable table{},before{};std::memset(&table,0xa5,sizeof(table));before=table;
 for(auto x:{0,-1,1025,INT_MIN,INT_MAX}){if(alpha::prepare_sprite_scale(x,16,&table)||std::memcmp(&table,&before,sizeof(table))){return 20;}++cases;if(alpha::prepare_sprite_scale(16,x,&table)||std::memcmp(&table,&before,sizeof(table))){return 20;}++cases;}
 if(alpha::prepare_sprite_scale(1,1,nullptr)){return 20;}++cases;
 if(!alpha::prepare_sprite_scale(11,16,&table)){return 20;}
 alpha::SpriteAxisPlan plan{},saved{};std::memset(&plan,0xa5,sizeof(plan));saved=plan;
 const std::int32_t bounds[][4]={{0,10,-1,3},{0,10,3,11},{10,0,3,4},{0,10,3,3},{0,10,4,3},{INT_MIN,INT_MAX,INT_MIN,INT_MAX},{0,801,799,801}};
 for(auto& b:bounds)for(bool expand:{false,true}){if(b[2]==799&&expand)continue;if(alpha::prepare_sprite_axis(table,expand,b[0],b[1],b[2],b[3],&plan)||std::memcmp(&plan,&saved,sizeof(plan))){return 20;}++cases;}
 if(alpha::prepare_sprite_axis(table,false,0,10,0,1,nullptr)){return 20;}++cases;
 table.steps[799]=0;if(alpha::prepare_sprite_axis(table,true,0,10,0,1,&plan)||std::memcmp(&plan,&saved,sizeof(plan))){return 20;}++cases;
 table.steps[799]=1025;if(alpha::prepare_sprite_axis(table,false,0,10,0,1,&plan)||std::memcmp(&plan,&saved,sizeof(plan))){return 20;}++cases;
 if(!alpha::prepare_sprite_scale(1,1,&table)){return 20;}
 if(alpha::prepare_sprite_axis(table,true,0,801,800,801,&plan)||std::memcmp(&plan,&saved,sizeof(plan))){return 20;}++cases;
 alignas(alpha::SpriteAxisPlan) unsigned char storage[sizeof(alpha::SpriteAxisPlan)+sizeof(alpha::SpriteScaleTable)+8],snapshot[sizeof(storage)];
 const unsigned overlaps[][2]={{0,0},{0,4},{0,3196},{4,0}};
 for(auto& offset:overlaps){
  std::memset(storage,0xa5,sizeof(storage));auto* t=new(storage+offset[0])alpha::SpriteScaleTable(table);std::memcpy(snapshot,storage,sizeof(storage));auto* p=reinterpret_cast<alpha::SpriteAxisPlan*>(storage+offset[1]);
  if(alpha::prepare_sprite_axis(*t,true,0,10,0,1,p)||std::memcmp(storage,snapshot,sizeof(storage))){return 20;}++cases;
 }
 std::printf("%u no-write boundary controls\n",cases);return 0;
}
