/* SPDX-License-Identifier: GPL-3.0-or-later; source policy field_attack_snapshot.h. */
#include "field_attack_snapshot.h"
#include "facility_production_snapshot.h"
#include "vehicle_stack.h"
#include "movement.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
struct Buffer {uint8_t*p=nullptr;~Buffer(){free(p);}};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,const FieldAttackStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&d,sizeof d)||overlap(out,n,&t,sizeof t)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
void put(uint8_t*p,uint32_t n,size_t bytes){for(size_t i=0;i<bytes;++i)p[i]=(uint8_t)(n>>(8*i));}
bool offset(const uint8_t*raw,size_t n,const void*p,size_t bytes,size_t&out){const uintptr_t a=(uintptr_t)raw,b=(uintptr_t)p;if(b<a||b-a>n||bytes>n-(size_t)(b-a))return false;out=(size_t)(b-a);return true;}
FieldAttackError convert(CombatError e){switch(e){case CombatError::ok:return FieldAttackError::ok;case CombatError::bad_buffer:return FieldAttackError::bad_buffer;case CombatError::overflow:return FieldAttackError::overflow;case CombatError::unsupported:return FieldAttackError::unsupported;default:return FieldAttackError::bad_state;}}
int32_t remap(int32_t id,int32_t loser){return id==loser?-1:id>loser?id-1:id;}
bool fit(int64_t n){return n>=0&&n<=2147483647LL;}
uint32_t roll(CombatRandomState&rng,int32_t strength,uint32_t&count){if(strength<=1)return 0;++count;return combat_crt_draw(rng)%(uint32_t)strength;}
bool promote(const OriginalWorld&w,const FieldCombatFacts&facts,const CombatResult&combat,
 size_t attacker,size_t defender,CombatRandomState&rng,uint32_t&draws,bool&promoted){
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);
 const bool win=combat.attacker_won;const auto*v=win?a:b;
 const int32_t morale=(int32_t)(win?facts.attacker_morale:facts.defender_morale);
 int64_t off=facts.duel.offense,def=facts.duel.defense;
 const int32_t ahp=10*facts.duel.attacker_reactor-a->damage_taken,dhp=10*facts.duel.defender_reactor-b->damage_taken;
 // Preserve source left-associated products; reject signed intermediate
 // overflow rather than letting host/68040 behavior diverge.
 off*=win?combat.attacker_hp:ahp;if(!fit(off))return false;
 off*=facts.duel.attacker_reactor;if(!fit(off))return false;
 off/=win?ahp:10*facts.duel.attacker_reactor;
 def*=win?dhp:combat.defender_hp;if(!fit(def))return false;
 def*=facts.duel.defender_reactor;if(!fit(def))return false;
 def/=win?10*facts.duel.defender_reactor:dhp;
 if(!fit(off+def))return false;
 const bool check=morale<=1||roll(rng,(int32_t)(off+def),draws)<=(uint32_t)(win?def:off);
 promoted=false;
 if(check&&v->morale<6&&morale<6){
  if(morale<=2||roll(rng,2,draws)==0)promoted=true;
 }
 return true;
}
}
size_t field_attack_snapshot_size(const OriginalWorld&w){return w.loaded()&&w.game().vehicle_count>=2&&w.original_size()>52?w.original_size()-52:0;}
FieldAttackError encode_field_attack_snapshot(const OriginalWorld&w,const UnitDefinitions&d,
 const Rules&t,const BasicRules&r,size_t attacker,size_t defender,const FieldAttackStage&stage,
 void*output,size_t capacity,FieldAttackReceipt&receipt){
 const size_t bytes=field_attack_snapshot_size(w);
 if(!bytes)return FieldAttackError::bad_state;
 if(!output||capacity<bytes||aliases(w,d,t,r,stage,output,bytes)||aliases(w,d,t,r,stage,&receipt,sizeof receipt)||overlap(output,bytes,&receipt,sizeof receipt))return FieldAttackError::bad_buffer;
 for(unsigned i=0;i<8;++i)if(stage.combat_wins[i]<0||stage.combat_losses[i]<0)return FieldAttackError::bad_state;
 FieldAttackReceipt next{};next.bytes=bytes;next.movement_random_seed=w.runtime_seed();next.next=stage;
 auto error=world_resolve_seeded_field_combat(w,d,t,r,attacker,defender,next.next.combat_random,next.facts,next.combat);
 if(error!=CombatError::ok)return convert(error);
 const auto&g=w.game();const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);
 if(g.expanded_preferences)return FieldAttackError::unsupported;
 if((a->flags&0x20u)||(b->flags&0x20u)||(a->state&(0x800001u))||(b->state&(0x800001u)))return FieldAttackError::unsupported;
 const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};size_t at,bt;
 if(movement_tile_index(geometry,a->x,a->y,at)!=LogicError::ok||movement_tile_index(geometry,b->x,b->y,bt)!=LogicError::ok)return FieldAttackError::bad_state;
 const size_t combat_tiles[2]={at,bt};
 for(size_t tile:combat_tiles){const int owner=w.tile(tile)->owner;if(owner>=0&&owner!=a->faction_id&&owner!=b->faction_id)return FieldAttackError::unsupported;}
 const int32_t loser=(int32_t)(next.combat.attacker_won?defender:attacker),winner=(int32_t)(next.combat.attacker_won?attacker:defender);
 const auto*lost=w.vehicle((size_t)loser);const auto*won=w.vehicle((size_t)winner);
 next.attacker_before=(int32_t)attacker;next.defender_before=(int32_t)defender;next.removed_before=loser;next.winner_after=remap(winner,loser);next.winner_faction=won->faction_id;next.loser_faction=lost->faction_id;
 const auto*lf=w.faction(lost->faction_id);const auto*wf=w.faction(won->faction_id);
 if(!lf||!wf||!lf->units_active[lost->unit_id]||lf->total_combat_units<1)return FieldAttackError::bad_state;
 uint32_t active=0,total=0;
 for(size_t i=0;i<g.vehicle_count;++i){const auto*v=w.vehicle(i);if(v->faction_id!=lost->faction_id)continue;const auto*p=w.prototype((size_t)v->unit_id);if(!p)return FieldAttackError::bad_state;if(v->unit_id==lost->unit_id)++active;if(p->plan<=7)++total;}
 if(active!=lf->units_active[lost->unit_id]||total!=(uint32_t)lf->total_combat_units||wf->diplo_combat_total[lost->faction_id]<0||wf->diplo_combat_count[lost->faction_id]<0)return FieldAttackError::bad_state;
 const auto monument_error=world_battle_monument(w,r,won->faction_id,lost->faction_id,next.monument);
 if(monument_error!=CombatError::ok)return convert(monument_error);
 if(a->moves_spent>255-r.move_rate_roads)return FieldAttackError::overflow;
 if(stage.combat_wins[won->faction_id]==2147483647||stage.combat_losses[lost->faction_id]==2147483647||wf->diplo_combat_total[lost->faction_id]==2147483647||wf->diplo_combat_count[lost->faction_id]==2147483647)return FieldAttackError::overflow;
 if(lf->units_lost[lost->unit_id]==65535u)return FieldAttackError::overflow;
 if(g.map_cursor[1]<-1||g.map_cursor[1]>=(int32_t)g.vehicle_count)return FieldAttackError::bad_reference;
 for(size_t i=0;i<g.vehicle_count;++i){const auto*v=w.vehicle(i);if(v->order==1&&(v->waypoint_x[0]<-1||v->waypoint_x[0]>=(int32_t)g.vehicle_count))return FieldAttackError::bad_reference;}
 if(!promote(w,next.facts,next.combat,attacker,defender,next.next.combat_random,next.promotion_draws,next.promoted))return FieldAttackError::overflow;
 ++next.next.combat_wins[won->faction_id];++next.next.combat_losses[lost->faction_id];
 next.selected_vehicle_after=remap(g.map_cursor[1],loser);next.recompute_required=true;next.selection_remap_required=true;next.presentation_required=true;
 Buffer scratch;scratch.p=(uint8_t*)malloc(w.original_size());if(!scratch.p)return FieldAttackError::out_of_memory;
 LocalSnapshotReceipt baseline_receipt{};const auto saved=encode_facility_production_snapshot(w,scratch.p,w.original_size(),baseline_receipt);
 if(saved!=LocalSnapshotError::ok)return saved==LocalSnapshotError::out_of_memory?FieldAttackError::out_of_memory:FieldAttackError::bad_snapshot;
 OriginalGameView before{};if(read_original_game(scratch.p,w.original_size(),before)!=GameReadError::ok)return FieldAttackError::bad_snapshot;
 size_t vehicle_offset,tile_offset,faction_offset,prototype_offset,monument_offset,ui_offset;
 if(!offset(scratch.p,w.original_size(),before.vehicles.data,before.vehicles.size,vehicle_offset)||!offset(scratch.p,w.original_size(),before.map.tiles,g.map.info.tile_count*44u,tile_offset)||!offset(scratch.p,w.original_size(),before.factions.data,before.factions.size,faction_offset)||!offset(scratch.p,w.original_size(),before.prototypes.data,before.prototypes.size,prototype_offset)||!offset(scratch.p,w.original_size(),before.monuments.data,before.monuments.size,monument_offset))return FieldAttackError::bad_snapshot;
 ui_offset=w.original_size()-248u;
 auto*wire=scratch.p;
 for(unsigned side=0;side<2;++side){const auto*v=side?b:a;const size_t id=side?defender:attacker;auto*p=wire+vehicle_offset+52u*id;
  put(p+4,v->state|4u|(side?0u:0x40u),4);p[16]=(uint8_t)(side?next.combat.defender_damage:next.combat.attacker_damage);
  if(!side){p[40]=(uint8_t)(a->moves_spent+r.move_rate_roads);p[39]|=(uint8_t)(1u<<b->faction_id);}
 }
 if(next.promoted)++wire[vehicle_offset+52u*(size_t)winner+36u];
 // Source veh_lift only clears occupancy for the unique casualty; the stale
 // tile owner nibble and all other original tile bytes remain unchanged.
 const size_t lost_tile=next.combat.attacker_won?bt:at;put(wire+tile_offset+44u*lost_tile+8u,w.tile(lost_tile)->items&~2u,4);
 const size_t fstride=0x20bcu;const size_t lbase=faction_offset+(size_t)lost->faction_id*fstride,wbase=faction_offset+(size_t)won->faction_id*fstride;
 --wire[lbase+0x854u+(size_t)lost->unit_id];put(wire+lbase+0xc54u+2u*(size_t)lost->unit_id,(uint32_t)lf->units_lost[lost->unit_id]+1u,2);put(wire+lbase+0x1054u,(uint32_t)(lf->total_combat_units-1),4);
 put(wire+wbase+0x18cu+4u*(size_t)lost->faction_id,(uint32_t)(wf->diplo_combat_total[lost->faction_id]+1),4);
 put(wire+wbase+0x1acu+4u*(size_t)lost->faction_id,(uint32_t)(wf->diplo_combat_count[lost->faction_id]+1),4);
 const uint32_t exclude=0x4000000u|0x1000000u|0x100000u|0x10000u|0x800u|0x400u|0x80u|7u|0x800000u;
 for(unsigned side=0;side<2;++side){const auto*v=side?b:a;const auto*other=side?a:b;const uint32_t old=w.faction(v->faction_id)->diplo_status[other->faction_id];put(wire+faction_offset+(size_t)v->faction_id*fstride+0x18u+4u*other->faction_id,(old&~exclude)|0x80008118u,4);}
 wire[prototype_offset+52u*(size_t)a->unit_id+0x2du]|=(uint8_t)(1u<<b->faction_id);wire[prototype_offset+52u*(size_t)b->unit_id+0x2du]|=(uint8_t)(1u<<a->faction_id);
 if(next.monument.changed){auto*event=wire+monument_offset+(size_t)won->faction_id*0x4f4u+6u*0x38u;put(event,(uint32_t)next.monument.year,4);put(event+8,(uint32_t)lost->faction_id,4);put(event+0xc,(uint32_t)next.monument.first_faction,4);put(event+0x10,1,4);put(event+0x14,(uint32_t)next.monument.first_achiever,4);put(wire+monument_offset+(size_t)won->faction_id*0x4f4u+0x498u,6,4);}
 for(size_t i=0;i<g.vehicle_count;++i)if((int32_t)i!=loser){const auto*v=w.vehicle(i);auto*p=wire+vehicle_offset+52u*i;
  if(v->prev_veh_id_stack>loser)put(p+50,(uint32_t)(v->prev_veh_id_stack-1),2);
  if(v->next_veh_id_stack>loser)put(p+48,(uint32_t)(v->next_veh_id_stack-1),2);
  if(v->order==1){if(v->waypoint_x[0]==loser)p[17]=0;else if(v->waypoint_x[0]>loser)put(p+20,(uint32_t)(v->waypoint_x[0]-1),2);}
 }
 put(wire+ui_offset+4u,(uint32_t)next.selected_vehicle_after,4);
 // Source count sits in the legacy preference block. Compact exactly the
 // removed52-byte record; every following base/scenario/opaque/UI byte shifts.
 put(wire+12u+0x38u,g.vehicle_count-1u,4);
 const size_t removed=vehicle_offset+52u*(size_t)loser;memmove(wire+removed,wire+removed+52u,w.original_size()-removed-52u);
 OriginalWorld after;const auto loaded=after.load(wire,bytes,w.runtime_seed());
 if(loaded!=WorldLoadError::ok)return loaded==WorldLoadError::out_of_memory?FieldAttackError::out_of_memory:FieldAttackError::bad_snapshot;
 VehicleStackStorage topology{geometry.width,geometry.height,const_cast<OriginalMapTile*>(after.tile(0)),after.game().map.info.tile_count,const_cast<OriginalVehicle*>(after.vehicle(0)),after.game().vehicle_count};
 const auto valid=validate_vehicle_stacks(topology);if(valid!=StackError::ok)return valid==StackError::out_of_memory?FieldAttackError::out_of_memory:FieldAttackError::bad_reference;
 if(after.game().vehicle_count!=g.vehicle_count-1u||after.game().map_cursor[1]!=next.selected_vehicle_after)return FieldAttackError::bad_snapshot;
 memcpy(output,wire,bytes);receipt=next;return FieldAttackError::ok;
}
const char* field_attack_error_name(FieldAttackError e){switch(e){case FieldAttackError::ok:return "ok";case FieldAttackError::bad_state:return "bad_state";case FieldAttackError::bad_buffer:return "bad_buffer";case FieldAttackError::unsupported:return "unsupported";case FieldAttackError::overflow:return "overflow";case FieldAttackError::out_of_memory:return "out_of_memory";case FieldAttackError::bad_snapshot:return "bad_snapshot";case FieldAttackError::bad_reference:return "bad_reference";}return "unknown";}
}
