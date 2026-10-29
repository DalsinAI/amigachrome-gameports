/* SPDX-License-Identifier: GPL-3.0-or-later. Policy and attribution in header. */
#include "drone_transition.h"
#include <stddef.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
void notice(DroneTransitionResult&r,DroneNotice n){r.notices[r.notice_count++]=n;}
uint16_t draw(CombatRandomState&rng,DroneTransitionResult&r){++r.random_draws;return combat_crt_draw(rng);}
}
DroneTransitionError drone_transition(const DroneTransitionFacts&f,CombatRandomState&rng,DroneTransitionResult&out){
 if(overlap(&f,sizeof f,&rng,sizeof rng)||overlap(&f,sizeof f,&out,sizeof out)||overlap(&rng,sizeof rng,&out,sizeof out))return DroneTransitionError::bad_buffer;
 if(f.population<0||f.population>127||f.talents<0||f.talents>f.population||f.drones<0||f.drones>f.population||
    f.talents+f.drones>f.population||f.superdrones<0||f.superdrones>f.drones||f.pending_police< -100||f.pending_police>100||f.assimilation_turns<0||f.assimilation_turns>255||
    f.owned_bases<1||f.owned_bases>512||f.difficulty<0||f.difficulty>5||f.ranking<0||f.ranking>7)return DroneTransitionError::bad_state;
 DroneTransitionResult r{};r.state_flags=f.state_flags;r.built=f.built;r.destroyed_facility=-1;auto next=rng;
 r.golden_age=!f.drones&&f.population>2&&f.talents>=(f.population+1)/2;
 if(r.golden_age){if(!(r.state_flags&4u)){r.state_flags|=4u;notice(r,DroneNotice::golden_started);}}
 else if(r.state_flags&4u){r.state_flags&=~4u;if(f.drones<=f.talents)notice(r,DroneNotice::golden_ended);}
 const bool manage=(f.governor_flags&0x80000040u)==0x80000040u;
 if(f.drones<=f.talents||f.nerve_stapled||f.telepathic_matrix){
  if(r.state_flags&2u){r.state_flags&=~2u;r.repaint_required=true;
   if(!(r.state_flags&4u))notice(r,manage?DroneNotice::riot_managed_end:DroneNotice::riot_ended);
  }
 }else{
  const bool first=!(r.state_flags&2u);r.state_flags|=2u;
  notice(r,first&&manage?DroneNotice::riot_managed_start:DroneNotice::riot_warning);
  if(first)r.repaint_required=true;
  else{
   // Preserve source evaluation order: even HQ/assimilation/one-base rejection
   // consumes the police draw when police>-6. Police<=-6 consumes none here.
   const bool police=f.pending_police<=-6||draw(next,r)%(f.pending_police+7)==0;
   if(police&&!(r.built&2u)&&f.assimilation_turns<45&&f.owned_bases>1){
    const bool skip=f.population<4&&draw(next,r)%3!=0;
    if(!skip){
     const int32_t modifier=f.population+f.difficulty+f.ranking+8;
     // Source increments iter after an unsuccessful attempt, then stops when
     // iter>modifier: exactly modifier+1 unsuccessful draws, not modifier.
     for(int32_t i=0;i<=modifier;++i){
      const unsigned id=draw(next,r)%39u;++r.facility_attempts;
      if(id>1&&id<34&&((r.built>>id)&1u)&&!((f.protected_facilities>>id)&1u)){
       r.built&=~(1ULL<<id);r.destroyed_facility=(int32_t)id;
       notice(r,DroneNotice::facility_destroyed);r.repaint_required=true;break;
      }
     }
    }
   }
  }
 }
 r.rioting=(r.state_flags&2u)!=0;
 r.recompute_required=r.state_flags!=f.state_flags||r.destroyed_facility>=0;
 rng=next;out=r;return DroneTransitionError::ok;
}
}
