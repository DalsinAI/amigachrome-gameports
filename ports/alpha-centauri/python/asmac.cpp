/* SPDX-License-Identifier: GPL-3.0-or-later
 * asmac: C API over alpha::play::Session (see asmac.h). New code. Every game
 * rule stays in the engine and the play layer; this file only converts
 * between the C structs and the C++ models and reads engine records for a
 * map view. Nothing here changes how the native game behaves.
 */
#include "asmac.h"
#include "play_session.h"
#include "original_world.h"
#include "engine.h"
#include "map.h"
#include "base_wp2.h"
#include "faction.h"
#include <memory>
#include <new>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace P=alpha::play;
namespace E=alpha::engine;

struct asmac_session {
 P::Session* s;bool owned;std::vector<uint8_t> buffer;
};
struct asmac_world { alpha::OriginalWorld w; };

#define COPYS(dst,src) snprintf((dst),sizeof(dst),"%s",(src))
template<class A,class B> static void copy_array(A& dst,const B& src){static_assert(sizeof(dst)==sizeof(src),"array size");memcpy(dst,src,sizeof(dst));}

static P::Session* S(asmac_session* h){h->s->bind();return h->s;}

/* Definitions keep the C linkage of their asmac.h declarations. */
int asmac_abi_version(void){return ASMAC_ABI_VERSION;}
const char* asmac_status_name(int s){return s==ASMAC_SHORT_BUFFER?"short buffer":P::play_status_name(s);}
const char* asmac_command_name(int k){return P::command_name(k);}
int asmac_parse_command(const char* line,asmac_command_t* out){
 P::Command c;const bool ok=P::parse_command(line,c);
 if(out){out->kind=c.kind;out->a=c.a;out->b=c.b;out->c=c.c;out->d=c.d;out->e=c.e;COPYS(out->text,c.text);}
 return ok?1:0;
}
int asmac_new_game_available(void){return P::Session::new_game_available()?1:0;}
int asmac_diplomacy_available(void){return P::Session::diplomacy_available()?1:0;}
uint32_t asmac_fnv1a(const uint8_t* p,size_t n,uint32_t h){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}

asmac_session* asmac_session_new(void){
 auto* h=new(std::nothrow) asmac_session();if(!h)return nullptr;
 h->s=new(std::nothrow) P::Session();if(!h->s){delete h;return nullptr;}
 h->owned=true;return h;
}
asmac_session* asmac_session_shared(void){
 static asmac_session shared{&P::session(),false,{}};return &shared;
}
void asmac_session_free(asmac_session* h){if(!h || !h->owned)return;delete h->s;delete h;}
int asmac_load_rules(asmac_session* h,const char* dir,int exp){return h->s->load_rules(dir?dir:"",exp);}
const char* asmac_rules_error(asmac_session* h){return h->s->rules_error();}
int asmac_new_game(asmac_session* h,uint32_t seed,int faction,int difficulty,int map_size){return h->s->new_game(seed,faction,difficulty,map_size);}
int asmac_load_game(asmac_session* h,const uint8_t* bytes,size_t n){if(!bytes)return ASMAC_BAD_ARGS;return h->s->load_game(bytes,n);}
int asmac_save_game(asmac_session* h,size_t* n){const int r=h->s->save_game(h->buffer);if(n)*n=h->buffer.size();return r;}
int asmac_view_bytes(asmac_session* h,size_t* n){const int r=h->s->view_bytes(h->buffer);if(n)*n=h->buffer.size();return r;}
int asmac_buffer(asmac_session* h,uint8_t* out,size_t cap){
 if(cap<h->buffer.size())return ASMAC_SHORT_BUFFER;
 if(!h->buffer.empty())memcpy(out,h->buffer.data(),h->buffer.size());
 return ASMAC_OK;
}
int asmac_loaded(asmac_session* h){return h->s->loaded()?1:0;}
int asmac_game_over(asmac_session* h){return h->s->game_over()?1:0;}
int asmac_run(asmac_session* h,const asmac_command_t* c){
 if(!c)return ASMAC_BAD_ARGS;
 P::Command x=P::command(c->kind,c->a,c->b,c->c,c->d,c->e);COPYS(x.text,c->text);
 return h->s->run(x);
}
void asmac_last_turn(asmac_session* h,asmac_turn_info_t* t){const P::TurnInfo& i=h->s->last_turn();t->status=i.status;t->faction=i.faction;t->turn=i.turn;t->year=i.year;t->events=i.events;}
int asmac_message_count(asmac_session* h){return (int)h->s->message_count();}
int asmac_message(asmac_session* h,int i,asmac_message_t* m){
 const P::PlayMessage* p=i>=0?h->s->message((size_t)i):nullptr;if(!p)return 0;
 m->turn=p->turn;m->kind=p->kind;COPYS(m->text,p->text);return 1;
}
void asmac_take_messages(asmac_session* h){h->s->take_messages();}
int asmac_player(asmac_session* h){return S(h)->player();}
int asmac_turn(asmac_session* h){return S(h)->turn();}
int asmac_year(asmac_session* h){return S(h)->year();}
int asmac_credits(asmac_session* h){return S(h)->credits();}
int asmac_vehicle_count(asmac_session* h){return S(h)->vehicle_count();}
int asmac_base_count(asmac_session* h){return S(h)->base_count();}
int asmac_unit(asmac_session* h,int v,asmac_unit_t* o){
 P::UnitInfo u;const bool ok=S(h)->unit(v,u);
 o->valid=u.valid;o->id=u.id;o->unit_id=u.unit_id;o->faction=u.faction;o->x=u.x;o->y=u.y;o->damage=u.damage;o->moves_left=u.moves_left;
 o->speed=u.speed;o->order=u.order;o->home_base=u.home_base;o->colony=u.colony;o->former=u.former;o->combat=u.combat;o->own=u.own;COPYS(o->name,u.name);
 return ok?1:0;
}
int asmac_next_unit(asmac_session* h,int v){return S(h)->next_unit(v);}
int asmac_unit_at(asmac_session* h,int x,int y){return S(h)->unit_at(x,y);}
int asmac_base_at(asmac_session* h,int x,int y){return S(h)->base_at(x,y);}
int asmac_step_target(asmac_session* h,int v,int d,int* x,int* y){int a=-1,b=-1;const bool ok=S(h)->step_target(v,d,a,b);if(x)*x=a;if(y)*y=b;return ok?1:0;}
int asmac_hostile_at(asmac_session* h,int x,int y){return S(h)->hostile_at(x,y)?1:0;}
int asmac_upgrade_target(asmac_session* h,int v){return S(h)->upgrade_target(v);}
int asmac_contacts(asmac_session* h,int* ids,int max){return S(h)->contacts(ids,max);}
uint32_t asmac_digest(asmac_session* h){return S(h)->digest();}

/* ---- engine records ---- */
int asmac_map_size(asmac_session* h,int* w,int* hgt){
 if(!S(h)->loaded()){if(w)*w=0;if(hgt)*hgt=0;return ASMAC_NO_GAME;}
 if(w)*w=*E::MapAreaX;
 if(hgt)*hgt=*E::MapAreaY;
 return ASMAC_OK;
}
static void fill_tile(int x,int y,asmac_tile_t* t){
 memset(t,0,sizeof(*t));t->base=-1;t->unit_faction=-1;t->owner=-1;
 E::MAP* sq=E::mapsq(x,y);
 if(!sq)return;
 t->valid=1;t->altitude=sq->climate>>5;t->rainfall=(sq->climate>>3)&3;t->temperature=sq->climate&7;t->rocky=sq->val3>>6;
 t->ocean=E::is_ocean(sq)?1:0;t->owner=sq->owner;t->region=sq->region;t->visibility=sq->visibility;t->items=sq->items;t->landmarks=sq->landmarks;
}
int asmac_tile(asmac_session* h,int x,int y,asmac_tile_t* t){
 if(!S(h)->loaded())return ASMAC_NO_GAME;
 fill_tile(x,y,t);if(!t->valid)return ASMAC_BAD_ARGS;
 t->base=E::base_at(x,y);
 for(int v=0;v<*E::VehCount;++v)if(E::Vehs[v].x==x && E::Vehs[v].y==y){if(!t->unit_count)t->unit_faction=E::Vehs[v].faction_id;++t->unit_count;}
 return ASMAC_OK;
}
int asmac_tiles(asmac_session* h,asmac_tile_t* out,int cap){
 if(!S(h)->loaded())return ASMAC_NO_GAME;
 const int w=*E::MapAreaX,ht=*E::MapAreaY;
 if(cap<w*ht)return ASMAC_SHORT_BUFFER;
 for(int y=0;y<ht;++y)for(int x=0;x<w;++x)fill_tile(x,y,out+y*w+x);
 for(int b=0;b<*E::BaseCount;++b){const int x=E::Bases[b].x,y=E::Bases[b].y;if(x>=0 && x<w && y>=0 && y<ht)out[y*w+x].base=b;}
 for(int v=0;v<*E::VehCount;++v){const int x=E::Vehs[v].x,y=E::Vehs[v].y;if(x<0 || x>=w || y<0 || y>=ht)continue;
  asmac_tile_t& t=out[y*w+x];if(!t.unit_count)t.unit_faction=E::Vehs[v].faction_id;++t.unit_count;}
 return ASMAC_OK;
}
static const char* item_name(int item){
 if(item>=0)return item<E::MaxProtoNum && E::Units[item].name[0]?E::Units[item].name:"Unit";
 if(-item<=E::SP_ID_Last && E::Facility[-item].name && E::Facility[-item].name[0])return E::Facility[-item].name;
 return "Facility";
}
const char* asmac_item_name(asmac_session* h,int item){if(!S(h)->loaded())return "";return item_name(item);}
int asmac_faction(asmac_session* h,int f,asmac_faction_t* o){
 memset(o,0,sizeof(*o));o->id=f;o->research_tech=-1;
 if(!S(h)->loaded() || f<0 || f>=E::MaxPlayerNum)return 0;
 E::Faction& F=E::Factions[f];E::MFaction& M=E::MFactions[f];
 o->valid=1;o->alive=f==0 || E::is_alive(f);o->human=f>0 && E::is_human(f);o->credits=F.energy_credits;
 for(int b=0;b<*E::BaseCount;++b)if(E::Bases[b].faction_id==f)++o->bases;
 for(int v=0;v<*E::VehCount;++v)if(E::Vehs[v].faction_id==f)++o->units;
 o->research_tech=F.tech_research_id;o->labs=F.labs_total;o->techs_known=F.tech_ranking/2;
 o->social[0]=F.SE_Politics_pending;o->social[1]=F.SE_Economics_pending;o->social[2]=F.SE_Values_pending;o->social[3]=F.SE_Future_pending;
 COPYS(o->noun,M.noun_faction);COPYS(o->formal_name,M.formal_name_faction);COPYS(o->leader,M.name_leader);COPYS(o->title,M.title_leader);
 snprintf(o->adjective,sizeof(o->adjective),"%.63s",M.adj_name_faction);
 return 1;
}
int asmac_base(asmac_session* h,int b,asmac_base_t* o){
 memset(o,0,sizeof(*o));o->id=b;
 if(!S(h)->loaded() || b<0 || b>=*E::BaseCount)return 0;
 E::BASE& B=E::Bases[b];o->valid=1;o->x=B.x;o->y=B.y;o->faction=B.faction_id;o->pop_size=B.pop_size;
 o->nutrients=B.nutrients_accumulated;o->minerals=B.minerals_accumulated;o->nutrient_surplus=B.nutrient_surplus;o->mineral_surplus=B.mineral_surplus;
 o->energy_surplus=B.energy_surplus;o->current_item=B.queue_items[0];o->queue_count=B.queue_size+1;
 o->cost=E::mineral_cost(b,B.queue_items[0]);o->turns=B.mineral_surplus>0?(o->cost-B.minerals_accumulated+B.mineral_surplus-1)/B.mineral_surplus:0;if(o->turns<0)o->turns=0;
 COPYS(o->name,B.name);COPYS(o->item_name,item_name(B.queue_items[0]));
 return 1;
}
int asmac_set_human(asmac_session* h,int f,int human){
 if(!S(h)->loaded())return ASMAC_NO_GAME;
 if(f<=0 || f>=E::MaxPlayerNum)return ASMAC_BAD_ARGS;
 if(human)E::FactionStatus[0]|=(uint8_t)(1u<<f);else E::FactionStatus[0]&=(uint8_t)~(1u<<f);
 return ASMAC_OK;
}

/* ---- panel models ---- */
static void to_c(const alpha::BaseProductionModel& m,asmac_production_t* o){
 o->valid=m.valid;o->base=m.base;COPYS(o->name,m.name);o->current=m.current;COPYS(o->current_name,m.current_name);
 o->minerals=m.minerals;o->cost=m.cost;o->surplus=m.surplus;o->turns=m.turns;o->queue_count=m.queue_count;
 for(int i=0;i<10;++i){o->queue[i]=m.queue[i];COPYS(o->queue_names[i],m.queue_names[i]);}
 o->hurry_cost=m.hurry_cost;o->credits=m.credits;o->hurried=m.hurried;o->count=m.count;
 for(int i=0;i<96;++i){o->items[i].id=m.items[i].id;o->items[i].kind=m.items[i].kind;COPYS(o->items[i].name,m.items[i].name);o->items[i].cost=m.items[i].cost;o->items[i].turns=m.items[i].turns;}
 o->selected=m.selected;
}
static void from_c(const asmac_production_t* o,alpha::BaseProductionModel& m){
 m=alpha::BaseProductionModel{};
 m.valid=o->valid!=0;m.base=o->base;COPYS(m.name,o->name);m.current=o->current;COPYS(m.current_name,o->current_name);
 m.minerals=o->minerals;m.cost=o->cost;m.surplus=o->surplus;m.turns=o->turns;m.queue_count=o->queue_count;
 for(int i=0;i<10;++i){m.queue[i]=o->queue[i];COPYS(m.queue_names[i],o->queue_names[i]);}
 m.hurry_cost=o->hurry_cost;m.credits=o->credits;m.hurried=o->hurried!=0;m.count=o->count;
 for(int i=0;i<96;++i){m.items[i].id=o->items[i].id;m.items[i].kind=o->items[i].kind;COPYS(m.items[i].name,o->items[i].name);m.items[i].cost=o->items[i].cost;m.items[i].turns=o->items[i].turns;}
 m.selected=o->selected;
}
static void to_c(const alpha::ResearchPanelModel& m,asmac_research_t* o){
 o->valid=m.valid;o->faction=m.faction;o->current=m.current;COPYS(o->current_name,m.current_name);o->accumulated=m.accumulated;o->cost=m.cost;
 o->labs=m.labs;o->known=m.known;o->count=m.count;copy_array(o->ids,m.ids);copy_array(o->names,m.names);copy_array(o->categories,m.categories);
 o->selected=m.selected;o->confirm=m.confirm;
}
static void from_c(const asmac_research_t* o,alpha::ResearchPanelModel& m){
 m=alpha::ResearchPanelModel{};
 m.valid=o->valid!=0;m.faction=o->faction;m.current=o->current;COPYS(m.current_name,o->current_name);m.accumulated=o->accumulated;m.cost=o->cost;
 m.labs=o->labs;m.known=o->known;m.count=o->count;copy_array(m.ids,o->ids);copy_array(m.names,o->names);copy_array(m.categories,o->categories);
 m.selected=o->selected;m.confirm=o->confirm!=0;
}
static void to_c(const alpha::SocialPanelModel& m,asmac_social_t* o){
 o->valid=m.valid;o->faction=m.faction;copy_array(o->categories,m.categories);copy_array(o->models,m.models);
 for(int c=0;c<4;++c){for(int k=0;k<4;++k){o->available[c][k]=m.available[c][k];o->cost[c][k]=m.cost[c][k];}o->current[c]=m.current[c];}
 o->credits=m.credits;o->row=m.row;o->col=m.col;
}
static void from_c(const asmac_social_t* o,alpha::SocialPanelModel& m){
 m=alpha::SocialPanelModel{};
 m.valid=o->valid!=0;m.faction=o->faction;copy_array(m.categories,o->categories);copy_array(m.models,o->models);
 for(int c=0;c<4;++c){for(int k=0;k<4;++k){m.available[c][k]=o->available[c][k]!=0;m.cost[c][k]=o->cost[c][k];}m.current[c]=o->current[c];}
 m.credits=o->credits;m.row=o->row;m.col=o->col;
}
static void to_c(const alpha::DesignPanelModel& m,asmac_design_t* o){
 static_assert(alpha::DESIGN_ROWS==5,"design rows");
 o->valid=m.valid;o->faction=m.faction;
 for(int r=0;r<5;++r){o->counts[r]=m.counts[r];o->selected[r]=m.selected[r];for(int i=0;i<32;++i){o->choices[r][i].id=m.choices[r][i].id;COPYS(o->choices[r][i].name,m.choices[r][i].name);}}
 o->row=m.row;o->preview_ok=m.preview_ok;COPYS(o->preview_name,m.preview_name);o->preview_cost=m.preview_cost;o->last_unit=m.last_unit;
}
static void from_c(const asmac_design_t* o,alpha::DesignPanelModel& m){
 m=alpha::DesignPanelModel{};
 m.valid=o->valid!=0;m.faction=o->faction;
 for(int r=0;r<5;++r){m.counts[r]=o->counts[r];m.selected[r]=o->selected[r];for(int i=0;i<32;++i){m.choices[r][i].id=o->choices[r][i].id;COPYS(m.choices[r][i].name,o->choices[r][i].name);}}
 m.row=o->row;m.preview_ok=o->preview_ok!=0;COPYS(m.preview_name,o->preview_name);m.preview_cost=o->preview_cost;m.last_unit=o->last_unit;
}
static void to_c(const alpha::BattlePanelModel& m,asmac_battle_t* o){
 o->valid=m.valid;o->preview=m.preview;o->x=m.x;o->y=m.y;COPYS(o->attacker,m.attacker);COPYS(o->defender,m.defender);
 COPYS(o->attacker_owner,m.attacker_owner);COPYS(o->defender_owner,m.defender_owner);o->odds=m.odds;o->outcome=m.outcome;COPYS(o->outcome_text,m.outcome_text);
 o->offense=m.offense;o->defense=m.defense;o->hp_atk_before=m.hp_atk_before;o->hp_def_before=m.hp_def_before;o->hp_atk_after=m.hp_atk_after;
 o->hp_def_after=m.hp_def_after;o->rounds=m.rounds;o->attacker_hits=m.attacker_hits;o->defender_hits=m.defender_hits;o->killed=m.killed;
 o->promoted_atk=m.promoted_atk;o->promoted_def=m.promoted_def;o->base_captured=m.base_captured;o->base_destroyed=m.base_destroyed;
 o->mod_count[0]=m.mod_count[0];o->mod_count[1]=m.mod_count[1];copy_array(o->mods,m.mods);
}
static void from_c(const asmac_battle_t* o,alpha::BattlePanelModel& m){
 m=alpha::BattlePanelModel{};
 m.valid=o->valid!=0;m.preview=o->preview!=0;m.x=o->x;m.y=o->y;COPYS(m.attacker,o->attacker);COPYS(m.defender,o->defender);
 COPYS(m.attacker_owner,o->attacker_owner);COPYS(m.defender_owner,o->defender_owner);m.odds=o->odds;m.outcome=o->outcome;COPYS(m.outcome_text,o->outcome_text);
 m.offense=o->offense;m.defense=o->defense;m.hp_atk_before=o->hp_atk_before;m.hp_def_before=o->hp_def_before;m.hp_atk_after=o->hp_atk_after;
 m.hp_def_after=o->hp_def_after;m.rounds=o->rounds;m.attacker_hits=o->attacker_hits;m.defender_hits=o->defender_hits;m.killed=o->killed;
 m.promoted_atk=o->promoted_atk!=0;m.promoted_def=o->promoted_def!=0;m.base_captured=o->base_captured!=0;m.base_destroyed=o->base_destroyed!=0;
 m.mod_count[0]=o->mod_count[0];m.mod_count[1]=o->mod_count[1];copy_array(m.mods,o->mods);
}
static void to_c(const alpha::DiploPanelModel& m,asmac_diplo_t* o){
 o->valid=m.valid;o->player=m.player;o->other=m.other;COPYS(o->other_name,m.other_name);COPYS(o->leader,m.leader);COPYS(o->mood,m.mood);COPYS(o->status,m.status);
 o->count=m.count;copy_array(o->proposals,m.proposals);copy_array(o->labels,m.labels);o->selected=m.selected;o->last_answer=m.last_answer;COPYS(o->reply,m.reply);
}
static void from_c(const asmac_diplo_t* o,alpha::DiploPanelModel& m){
 m=alpha::DiploPanelModel{};
 m.valid=o->valid!=0;m.player=o->player;m.other=o->other;COPYS(m.other_name,o->other_name);COPYS(m.leader,o->leader);COPYS(m.mood,o->mood);COPYS(m.status,o->status);
 m.count=o->count;copy_array(m.proposals,o->proposals);copy_array(m.labels,o->labels);m.selected=o->selected;m.last_answer=o->last_answer;COPYS(m.reply,o->reply);
}

/* Model round trip through the C struct: from_c, call, to_c. The C++ models
 * are large, so they live on the heap. */
template<class M,class C,class F> static auto with_model(C* o,F f)->decltype(f(*(M*)nullptr)){
 std::unique_ptr<M> m(new M());from_c(o,*m);auto r=f(*m);to_c(*m,o);return r;
}
int asmac_base_production(asmac_session* h,int b,asmac_production_t* o){return with_model<alpha::BaseProductionModel>(o,[&](alpha::BaseProductionModel& m){return (int)S(h)->base_production(b,m);});}
int asmac_research(asmac_session* h,asmac_research_t* o){return with_model<alpha::ResearchPanelModel>(o,[&](alpha::ResearchPanelModel& m){return (int)S(h)->research(m);});}
int asmac_social(asmac_session* h,asmac_social_t* o){return with_model<alpha::SocialPanelModel>(o,[&](alpha::SocialPanelModel& m){return (int)S(h)->social(m);});}
int asmac_design(asmac_session* h,asmac_design_t* o){return with_model<alpha::DesignPanelModel>(o,[&](alpha::DesignPanelModel& m){return (int)S(h)->design(m);});}
void asmac_design_preview(asmac_session* h,asmac_design_t* o){with_model<alpha::DesignPanelModel>(o,[&](alpha::DesignPanelModel& m){S(h)->design_preview(m);return 0;});}
void asmac_design_selection(const asmac_design_t* o,int* c,int* w,int* a,int* r,int* ab){
 std::unique_ptr<alpha::DesignPanelModel> m(new alpha::DesignPanelModel());from_c(o,*m);int x[5];alpha::design_selection(*m,x[0],x[1],x[2],x[3],x[4]);
 *c=x[0];*w=x[1];*a=x[2];*r=x[3];*ab=x[4];
}
void asmac_battle_model(asmac_session* h,asmac_battle_t* o){to_c(S(h)->battle(),o);}
int asmac_diplomacy(asmac_session* h,int other,asmac_diplo_t* o){return with_model<alpha::DiploPanelModel>(o,[&](alpha::DiploPanelModel& m){return (int)S(h)->diplomacy(other,m);});}
int asmac_ui_key_from_char(int c){return alpha::ui_key_from_char(c);}
int asmac_base_production_key(asmac_production_t* o,int c){return with_model<alpha::BaseProductionModel>(o,[&](alpha::BaseProductionModel& m){return alpha::base_production_key(m,c);});}
int asmac_research_key(asmac_research_t* o,int k){return with_model<alpha::ResearchPanelModel>(o,[&](alpha::ResearchPanelModel& m){return alpha::research_panel_key(m,k);});}
int asmac_social_key(asmac_social_t* o,int k){return with_model<alpha::SocialPanelModel>(o,[&](alpha::SocialPanelModel& m){return alpha::social_panel_key(m,k);});}
int asmac_design_key(asmac_design_t* o,int k){return with_model<alpha::DesignPanelModel>(o,[&](alpha::DesignPanelModel& m){return alpha::design_panel_key(m,k);});}
int asmac_battle_key(const asmac_battle_t* o,int k){std::unique_ptr<alpha::BattlePanelModel> m(new alpha::BattlePanelModel());from_c(o,*m);return alpha::battle_panel_key(*m,k);}
int asmac_diplo_key(asmac_diplo_t* o,int k){return with_model<alpha::DiploPanelModel>(o,[&](alpha::DiploPanelModel& m){return alpha::diplo_panel_key(m,k);});}

/* ---- renderers ---- */
template<class F> static int render(const asmac_frame_t* f,const asmac_rect_t* r,asmac_text_item_t* out,int cap,F draw){
 if(!f || !r || cap<0)return -1;
 const alpha::UiFrame frame{f->rgb,f->capacity,f->width,f->height};const alpha::UiRect rect{r->x,r->y,r->w,r->h};
 std::vector<alpha::UiTextItem> items((size_t)(cap>0?cap:1));
 const size_t n=draw(frame,rect,cap>0?items.data():nullptr,(size_t)cap);
 for(size_t i=0;i<n && (int)i<cap;++i){out[i].x=items[i].x;out[i].baseline=items[i].baseline;out[i].width=items[i].width;out[i].ink=items[i].ink;
  out[i].heading=items[i].heading;COPYS(out[i].text,items[i].text);}
 return (int)n;
}
#define RENDER(NAME,CT,MT,FN) \
int NAME(const CT* o,const asmac_frame_t* f,const asmac_rect_t* r,asmac_text_item_t* out,int cap){ \
 std::unique_ptr<MT> m(new MT());from_c(o,*m); \
 return render(f,r,out,cap,[&](const alpha::UiFrame& fr,const alpha::UiRect& re,alpha::UiTextItem* it,size_t c){return FN(*m,fr,re,it,c);});}
RENDER(asmac_render_base_production,asmac_production_t,alpha::BaseProductionModel,alpha::render_base_production)
RENDER(asmac_render_research,asmac_research_t,alpha::ResearchPanelModel,alpha::render_research_panel)
RENDER(asmac_render_social,asmac_social_t,alpha::SocialPanelModel,alpha::render_social_panel)
RENDER(asmac_render_design,asmac_design_t,alpha::DesignPanelModel,alpha::render_design_panel)
RENDER(asmac_render_battle,asmac_battle_t,alpha::BattlePanelModel,alpha::render_battle_panel)
RENDER(asmac_render_diplo,asmac_diplo_t,alpha::DiploPanelModel,alpha::render_diplo_panel)

/* ---- OriginalWorld ---- */
asmac_world* asmac_world_load(const uint8_t* bytes,size_t n,uint32_t seed,int* error){
 auto* w=new(std::nothrow) asmac_world();if(!w){if(error)*error=(int)alpha::WorldLoadError::out_of_memory;return nullptr;}
 const auto e=w->w.load(bytes,n,seed);if(error)*error=(int)e;
 if(e!=alpha::WorldLoadError::ok){delete w;return nullptr;}
 return w;
}
const char* asmac_world_error_name(int e){return alpha::world_load_error_name((alpha::WorldLoadError)e);}
void asmac_world_free(asmac_world* w){delete w;}
void asmac_world_info_get(const asmac_world* w,asmac_world_info_t* o){
 const auto& g=w->w.game();o->vehicle_count=(int)g.vehicle_count;o->base_count=(int)g.base_count;o->current_turn=(int)g.preferences.CurrentTurn;o->player=(int)g.player;
}
int asmac_world_vehicle(const asmac_world* w,int i,int* x,int* y,int* unit_id){
 const auto* v=i>=0?w->w.vehicle((size_t)i):nullptr;if(!v)return 0;*x=v->x;*y=v->y;*unit_id=v->unit_id;return 1;
}
int asmac_world_base_faction(const asmac_world* w,int i){const auto* b=i>=0?w->w.base((size_t)i):nullptr;return b?b->faction_id:-1;}
int asmac_world_render_base_window(const asmac_world* w,int base,int viewer,const asmac_frame_t* f,const asmac_rect_t* r,asmac_text_item_t* out,int cap){
 if(base<0)return -1;
 std::unique_ptr<alpha::BaseWindowModel> m(new alpha::BaseWindowModel());
 if(!alpha::build_base_window(w->w,(size_t)base,nullptr,nullptr,viewer,*m))return -1;
 return render(f,r,out,cap,[&](const alpha::UiFrame& fr,const alpha::UiRect& re,alpha::UiTextItem* it,size_t c){return alpha::render_base_window(*m,fr,re,it,c);});
}

