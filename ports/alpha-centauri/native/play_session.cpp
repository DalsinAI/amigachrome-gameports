/* SPDX-License-Identifier: GPL-3.0-or-later
 * Play command layer over the engine (see play_session.h). New code: every
 * game rule is the engine's (WP1 turn, WP2 base, WP3 orders, WP4 combat,
 * WP5 research/social/design, WP7 new game, WP8 diplomacy); this file only
 * checks ownership, calls the package entry points, turns engine events into
 * message-log lines and fills the panel models.
 */
#include "play_session.h"
#include "engine.h"
#include "turn.h"
#include "integration.h"
#include "base_wp2.h"
#include "veh_action.h"
#include "combat.h"
#include "tech.h"
#include "design.h"
#if __has_include("engine/newgame.h")
#include "newgame.h"
#define PLAY_HAS_NEWGAME 1
#else
#define PLAY_HAS_NEWGAME 0
#endif
#if __has_include("engine/diplo.h") && __has_include("ui_diplo.h")
#include "diplo.h"
#include "ui_diplo.h"
#define PLAY_HAS_DIPLO 1
#else
#define PLAY_HAS_DIPLO 0
#endif
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace alpha {
namespace play {
namespace E = alpha::engine;

enum { MK_INFO=0,MK_ACTION=1,MK_WARNING=2,MK_COMBAT=3,MK_BASE=4,MK_RESEARCH=5 };

const char* play_status_name(int s){
 static const char* const names[]={"ok","refused","bad arguments","no game","not in this build","file error","game over","attack needed","needs confirmation"};
 return s>=0 && s<(int)(sizeof(names)/sizeof(names[0]))?names[s]:"unknown";
}

static const char* const command_words[CMD_COUNT]={"none","move","goto","roadto","terraform","build","sentry","hold","skip","wake",
 "disband","sethome","returnhome","upgrade","odds","attack","produce","queueadd","queueremove","queuemove","hurry","research",
 "social","design","diplo","endturn","editunit"};
const char* command_name(int kind){return kind>=0 && kind<CMD_COUNT?command_words[kind]:"?";}
Command command(int kind,int a,int b,int c,int d,int e){Command x;x.kind=kind;x.a=a;x.b=b;x.c=c;x.d=d;x.e=e;x.text[0]=0;return x;}

bool parse_command(const char* line,Command& out){
 out=command(CMD_NONE);if(!line)return false;
 char word[24];int n=0;
 while(*line==' ' || *line=='\t')++line;
 while(*line && *line!=' ' && *line!='\t' && n<23)word[n++]=*line++;
 word[n]=0;
 int kind=-1;for(int k=1;k<CMD_COUNT;++k)if(!strcmp(word,command_words[k]))kind=k;
 if(kind<0)return false;
 int v[5]={0,0,0,0,0},count=0;
 while(count<5){while(*line==' ' || *line=='\t')++line;if(!*line)break;
  char* end=nullptr;long x=strtol(line,&end,10);if(end==line)break;v[count++]=(int)x;line=end;}
 static const int need[CMD_COUNT]={0,2,3,3,2,1,1,1,1,1,1,1,1,1,3,3,2,2,2,3,1,1,2,5,2,0,4};
 if(count<need[kind])return false;
 out=command(kind,v[0],v[1],v[2],v[3],v[4]);
 if(kind==CMD_ROAD_TO && count<4)out.d=0;
 if(kind==CMD_UPGRADE && count<2)out.b=-1;
 while(*line==' ' || *line=='\t')++line;
 snprintf(out.text,sizeof(out.text),"%s",line);
 size_t len=strlen(out.text);while(len && (out.text[len-1]=='\n' || out.text[len-1]=='\r' || out.text[len-1]==' '))out.text[--len]=0;
 return true;
}

class SessionImpl;
/* Event sink: names are read while the engine raises the event, so the
 * messages keep them even when ids shift afterwards. During End Turn the
 * lines are queued and matched to the TurnResult events in order. */
class PlaySink : public E::EventSink {
public:
 SessionImpl* s=nullptr;
 int popup(const E::PopupRequest&) override;
 void redraw(const E::RedrawRequest&) override {}
 void notify(const E::EngineEvent&) override;
};

class SessionImpl {
public:
 std::unique_ptr<E::EngineState> state;
 PlaySink sink;
 std::vector<PlayMessage> messages;
 TurnInfo last{};
 BattlePanelModel battle{};
 bool loaded=false,over=false,in_turn=false;
 std::vector<std::string> live; // formatted sink events during End Turn ("" = not shown)
 std::vector<uint8_t> live_kind;
 int preferred_tech=-1;
 std::string rules_error;
#if PLAY_HAS_DIPLO
 ui_diplo::DiploDialog dialog{};bool dialog_open=false;
#endif

 SessionImpl():state(new E::EngineState()){E::engine_reset(*state);sink.s=this;}
 void add(int kind,const char* fmt,...) __attribute__((format(printf,3,4))){
  PlayMessage m;m.turn=loaded?*E::CurrentTurn:0;m.kind=(uint8_t)kind;
  va_list ap;va_start(ap,fmt);vsnprintf(m.text,sizeof(m.text),fmt,ap);va_end(ap);
  if(m.text[0])messages.push_back(m);
 }
 int player()const{return loaded?E::MapWin->cOwner:-1;}
 bool own_veh(int v)const{return loaded && v>=0 && v<*E::VehCount && E::Vehs[v].faction_id==player();}
 bool own_base(int b)const{return loaded && b>=0 && b<*E::BaseCount && E::Bases[b].faction_id==player();}
 static const char* noun(int f){if(f<=0 || f>=E::MaxPlayerNum)return "Planet";const char* s=E::MFactions[f].noun_faction;return s[0]?s:"Faction";}
 static const char* unit_name(int unit_id){return unit_id>=0 && unit_id<E::MaxProtoNum && E::Units[unit_id].name[0]?E::Units[unit_id].name:"Unit";}
 static const char* item_name(int item){
  if(item>=0)return unit_name(item);
  if(-item<=E::SP_ID_Last && E::Facility[-item].name && E::Facility[-item].name[0])return E::Facility[-item].name;
  return "Facility";
 }
 std::string describe_popup(const E::PopupRequest& r){
  char line[120];
  if(r.strings[0][0])snprintf(line,sizeof(line),"%s: %.80s",r.label,r.strings[0]);else snprintf(line,sizeof(line),"%s",r.label);
  return line;
 }
 std::string describe_event(const E::EngineEvent& e,uint8_t& kind){
  char line[120];line[0]=0;kind=MK_INFO;const int me=player();
  switch(e.type){
  case E::EV_VEH_CREATED:
   if(e.c==me){kind=MK_BASE;snprintf(line,sizeof(line),"New unit: %s at %d, %d",unit_name(e.b),e.x,e.y);}break;
  case E::EV_VEH_KILLED:
   if(e.c==me){kind=MK_COMBAT;snprintf(line,sizeof(line),"Unit lost: %s at %d, %d",unit_name(e.b),e.x,e.y);}break;
  case E::EV_BASE_CREATED:
   kind=MK_BASE;
   if(e.a>=0 && e.a<*E::BaseCount)snprintf(line,sizeof(line),"%s base founded: %s",noun(e.c),E::Bases[e.a].name);
   break;
  case E::EV_BASE_KILLED:
   kind=MK_BASE;snprintf(line,sizeof(line),"%s base at %d, %d destroyed",noun(e.c),e.x,e.y);break;
  case E::EV_GAME_LOADED:snprintf(line,sizeof(line),"Game loaded");break;
  case E::EV_GAME_SAVED:snprintf(line,sizeof(line),"Game saved");break;
  case E::EV_MESSAGE:
   if(e.text && !strncmp(e.text,"ui:",3))break;           // window/sound requests of WP2
   if(e.text && e.a>=E::TE_TURN_BEGIN && e.a<=E::TE_TERRITORY)break; // phase markers: from TurnResult
   if(e.text && e.text[0])snprintf(line,sizeof(line),"%.110s",e.text);
   break;
  default:break; // EV_NOT_IMPLEMENTED is counted per End Turn, EV_MAP_CHANGED needs no line
  }
  return line;
 }
 void on_popup(const E::PopupRequest& r){
  std::string text=describe_popup(r);
  if(in_turn){live.push_back(text);live_kind.push_back(MK_INFO);}
  else add(MK_INFO,"%s",text.c_str());
 }
 void on_notify(const E::EngineEvent& e){
  uint8_t kind;std::string text=describe_event(e,kind);
  if(in_turn){live.push_back(text);live_kind.push_back(kind);}
  else if(!text.empty())add(kind,"%s",text.c_str());
 }

 void bind(){
  E::engine_bind(state.get());
  E::set_event_sink(&sink);
  E::engine_install_turn_hooks();
  E::TurnHooks& h=E::turn_hooks();
  h.interlude=&interlude;
#if PLAY_HAS_NEWGAME
  if(!h.crash_landing)h.crash_landing=E::crash_landing;
#endif
#if PLAY_HAS_DIPLO
  if(!h.pact_ends)h.pact_ends=E::pact_ends;
  if(!h.can_call_council)h.can_call_council=E::can_call_council;
  if(!h.call_council)h.call_council=E::call_council;
  // Comm-link contacts the engine opens during End Turn close at once; the
  // player opens the dialog from the map (F5).
  ui_diplo::install_ui_diplo(nullptr,nullptr,0);
#endif
  E::set_tech_chooser(&choose_tech,this);
 }
 static void interlude(int event_id,const char* text,int,int){
  if(current)current->add(MK_INFO,"Interlude %d%s%.80s",event_id,text && text[0]?": ":"",text?text:"");
 }
 static SessionImpl* current;
 static int choose_tech(const E::TechChoice& c,void* ctx){
  SessionImpl* s=(SessionImpl*)ctx;if(!c.count)return -1;
  int pick=c.ids[0];
  for(int i=0;i<c.count;++i)if(c.ids[i]==s->preferred_tech)pick=c.ids[i];
  const char* name=pick>=0 && pick<E::MaxTechnologyNum && E::Tech[pick].name?E::Tech[pick].name:"?";
  if(s->in_turn){s->live.push_back(std::string("Research goal: ")+name);s->live_kind.push_back(MK_RESEARCH);}
  else s->add(MK_RESEARCH,"Research goal: %s",name);
  return pick;
 }

 void after_load(){
  loaded=true;over=false;battle=BattlePanelModel{};
  last=TurnInfo{E::TURN_HUMAN,player(),*E::CurrentTurn,*E::CurrentMissionYear,0};
 }
 int end_turn();
 void continue_orders();
 int run(const Command&);
 void fill_battle_preview(int veh,int x,int y);
 void fill_battle_result(const E::BattleResult&);
};
SessionImpl* SessionImpl::current=nullptr;

int PlaySink::popup(const E::PopupRequest& r){s->on_popup(r);return 0;}
void PlaySink::notify(const E::EngineEvent& e){s->on_notify(e);}

/* Units with standing orders move on at the start of the human turn, as the
 * original's human turn did before handing control to the player. */
void SessionImpl::continue_orders(){
 const int me=player();
 for(int v=0;v<*E::VehCount;++v){
  E::VEH& veh=E::Vehs[v];
  if(veh.faction_id!=me)continue;
  const int o=veh.order;
  if(o==E::ORDER_NONE || o==E::ORDER_SENTRY_BOARD || o==E::ORDER_HOLD)continue;
  const int count=*E::VehCount;
  E::order_continue(v);
  if(*E::VehCount<count)--v; // the unit was removed; ids above it shifted
 }
}

int SessionImpl::end_turn(){
 if(!loaded)return PLAY_NO_GAME;
 if(over)return PLAY_GAME_OVER;
 live.clear();live_kind.clear();in_turn=true;
 E::TurnResult r=E::engine_end_turn();
 in_turn=false;
 size_t next=0;int ai=0,unimplemented=0;
 for(const E::TurnEvent& e:r.events){
  const bool from_sink=e.kind==E::TE_ENGINE || e.kind==E::TE_POPUP || e.kind==E::TE_TURN_BEGIN || e.kind==E::TE_FACTION_UPKEEP
   || e.kind==E::TE_AI_TURN || e.kind==E::TE_TERRITORY;
  std::string text;uint8_t kind=MK_INFO;
  if(from_sink && next<live.size()){text=live[next];kind=live_kind[next];++next;
   // A research choice made by the chooser was queued in between.
   while(next<live.size() && live_kind[next]==MK_RESEARCH && !strncmp(live[next].c_str(),"Research goal",13)){
    PlayMessage m;m.turn=e.turn;m.kind=MK_RESEARCH;snprintf(m.text,sizeof(m.text),"%s",live[next].c_str());messages.push_back(m);++next;}}
  char line[120];line[0]=0;
  switch(e.kind){
  case E::TE_TURN_BEGIN:snprintf(line,sizeof(line),"Turn %d begins, mission year %d",e.turn,e.a);break;
  case E::TE_AI_TURN:++ai;break;
  case E::TE_FACTION_UPKEEP:case E::TE_TERRITORY:break;
  case E::TE_HUMAN_TURN_END:snprintf(line,sizeof(line),"%s turn ended",noun(e.faction_id));break;
  case E::TE_HUMAN_TURN:snprintf(line,sizeof(line),"Your turn: %s, turn %d",noun(e.faction_id),e.turn);break;
  case E::TE_ROUND_END:snprintf(line,sizeof(line),"Round ended without a human turn");break;
  case E::TE_GAME_OVER:snprintf(line,sizeof(line),"GAME OVER");kind=MK_WARNING;break;
  case E::TE_ENGINE:if(e.type==E::EV_NOT_IMPLEMENTED){++unimplemented;break;}
   snprintf(line,sizeof(line),"%s",text.c_str());break;
  case E::TE_POPUP:snprintf(line,sizeof(line),"%s",text.c_str());break;
  default:break;}
  if(line[0]){PlayMessage m;m.turn=e.turn;m.kind=kind;snprintf(m.text,sizeof(m.text),"%s",line);messages.push_back(m);}
 }
 while(next<live.size()){if(!live[next].empty())add(live_kind[next],"%s",live[next].c_str());++next;}
 if(ai)add(MK_INFO,"%d computer faction turn%s played",ai,ai==1?"":"s");
 if(unimplemented)add(MK_WARNING,"%d engine routine call%s not available yet",unimplemented,unimplemented==1?"":"s");
 last=TurnInfo{r.status,r.faction_id,r.turn,r.year,(int)r.events.size()};
 if(r.status==E::TURN_GAME_OVER){over=true;return PLAY_GAME_OVER;}
 if(r.status==E::TURN_HUMAN)continue_orders();
 return PLAY_OK;
}

void SessionImpl::fill_battle_preview(int veh,int x,int y){
 battle=BattlePanelModel{};battle.valid=true;battle.preview=true;battle.x=x;battle.y=y;
 int def=-1;battle.odds=E::engine_attack_odds(veh,x,y,&def);
 snprintf(battle.attacker,sizeof(battle.attacker),"%s",unit_name(E::Vehs[veh].unit_id));
 snprintf(battle.attacker_owner,sizeof(battle.attacker_owner),"%s",noun(E::Vehs[veh].faction_id));
 if(def>=0 && def<*E::VehCount){snprintf(battle.defender,sizeof(battle.defender),"%s",unit_name(E::Vehs[def].unit_id));
  snprintf(battle.defender_owner,sizeof(battle.defender_owner),"%s",noun(E::Vehs[def].faction_id));battle.hp_def_before=E::Vehs[def].damage_taken;}
 else{const int b=E::base_at(x,y);snprintf(battle.defender,sizeof(battle.defender),"%s",b>=0?E::Bases[b].name:"nothing");
  snprintf(battle.defender_owner,sizeof(battle.defender_owner),"%s",b>=0?noun(E::Bases[b].faction_id):"-");}
 battle.hp_atk_before=E::Vehs[veh].damage_taken;
}

void SessionImpl::fill_battle_result(const E::BattleResult& r){
 battle.preview=false;battle.valid=true;battle.outcome=r.outcome;
 snprintf(battle.outcome_text,sizeof(battle.outcome_text),"%s",E::battle_outcome_name(r.outcome));
 if(r.unit_id_def>=0){snprintf(battle.defender,sizeof(battle.defender),"%s",unit_name(r.unit_id_def));snprintf(battle.defender_owner,sizeof(battle.defender_owner),"%s",noun(r.faction_id_def));}
 battle.offense=r.offense;battle.defense=r.defense;battle.hp_atk_before=r.hp_atk_before;battle.hp_def_before=r.hp_def_before;
 battle.hp_atk_after=r.hp_atk_after;battle.hp_def_after=r.hp_def_after;battle.rounds=r.rounds;battle.attacker_hits=r.attacker_hits;
 battle.defender_hits=r.defender_hits;battle.killed=r.num_killed;battle.promoted_atk=r.promoted_atk;battle.promoted_def=r.promoted_def;
 battle.base_captured=r.base_captured;battle.base_destroyed=r.base_destroyed;
 for(int s=0;s<2;++s){battle.mod_count[s]=r.mod_count[s]<4?r.mod_count[s]:4;
  for(int i=0;i<battle.mod_count[s];++i)snprintf(battle.mods[s][i],sizeof(battle.mods[s][i]),"%+d%% %.40s",r.mods[s][i].percent,r.mods[s][i].text);}
}

static bool veh_hostile_tile(int me,int x,int y){
 for(int v=0;v<*E::VehCount;++v){const E::VEH& o=E::Vehs[v];if(o.x==x && o.y==y && o.faction_id!=me && !E::has_pact(me,o.faction_id))return true;}
 const int b=E::base_at(x,y);
 return b>=0 && E::Bases[b].faction_id!=me && !E::has_pact(me,E::Bases[b].faction_id);
}

/* The prototype a unit would upgrade to: the strongest buildable design of
 * the same triad and plan that is stronger than the unit's own. */
static int best_upgrade(int me,int v){
 int best=-1,score=-1;const int old=E::Vehs[v].unit_id;const int triad=E::Chassis[E::Units[old].chassis_id].triad;
 const int own=E::proto_offense(old)*64+E::proto_defense(old);
 for(int u=0;u<E::MaxProtoNum;++u){if(u==old || !(E::Units[u].unit_flags&E::UNIT_ACTIVE) || !E::veh_avail(u,me,-1))continue;
  if(E::Chassis[E::Units[u].chassis_id].triad!=triad || E::Units[u].plan!=E::Units[old].plan)continue;
  const int s=E::proto_offense(u)*64+E::proto_defense(u);
  if(s>own && s>score){score=s;best=u;}}
 return best;
}

static int plan_for(int chassis,int weapon,int armor){
 (void)chassis;(void)weapon;(void)armor;
 return E::PLAN_AUTO_CALCULATE;
}

int SessionImpl::run(const Command& c){
 if(!loaded)return PLAY_NO_GAME;
 SessionImpl::current=this;
 if(over && c.kind!=CMD_ODDS)return PLAY_GAME_OVER;
 const int me=player();
 switch(c.kind){
 case CMD_MOVE:{
  if(!own_veh(c.a) || c.b<0 || c.b>7)return PLAY_BAD_ARGS;
  const int x=E::wrap(E::Vehs[c.a].x+E::BaseOffsetX[c.b]),y=E::Vehs[c.a].y+E::BaseOffsetY[c.b];
  if(!E::mapsq(x,y))return PLAY_REFUSED;
  if(veh_hostile_tile(me,x,y))return PLAY_ATTACK; // attacks go through engine_attack (CMD_ATTACK)
  const int ox=E::Vehs[c.a].x,oy=E::Vehs[c.a].y;
  if(!E::order_move(c.a,c.b))return PLAY_REFUSED;
  if(c.a<*E::VehCount && (E::Vehs[c.a].x!=ox || E::Vehs[c.a].y!=oy))add(MK_ACTION,"%s moved to %d, %d",unit_name(E::Vehs[c.a].unit_id),E::Vehs[c.a].x,E::Vehs[c.a].y);
  return PLAY_OK;}
 case CMD_GOTO:
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  if(!E::order_go_to(c.a,c.b,c.c))return PLAY_REFUSED;
  add(MK_ACTION,"Go to %d, %d",c.b,c.c);return PLAY_OK;
 case CMD_ROAD_TO:
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  if(!E::order_road_to(c.a,c.b,c.c,c.d!=0))return PLAY_REFUSED;
  add(MK_ACTION,"%s to %d, %d",c.d?"Mag tube":"Road",c.b,c.c);return PLAY_OK;
 case CMD_TERRAFORM:
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  if(!E::order_terraform(c.a,c.b))return PLAY_REFUSED;
  add(MK_ACTION,"Terraforming: %s",c.b>=0 && c.b<E::MaxTerrainNum && E::Terraform[c.b].name?E::Terraform[c.b].name:"order");return PLAY_OK;
 case CMD_BUILD_BASE:{
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  const int before=*E::BaseCount;
  const int r=E::order_build_base(c.a,c.text[0]?c.text:nullptr);
  if(r<0 || *E::BaseCount==before)return PLAY_REFUSED;
  return PLAY_OK;}
 case CMD_SENTRY:if(!own_veh(c.a))return PLAY_BAD_ARGS;return E::order_sentry(c.a)?PLAY_OK:PLAY_REFUSED;
 case CMD_HOLD:if(!own_veh(c.a))return PLAY_BAD_ARGS;return E::order_hold(c.a)?PLAY_OK:PLAY_REFUSED;
 case CMD_SKIP:if(!own_veh(c.a))return PLAY_BAD_ARGS;return E::order_skip(c.a)?PLAY_OK:PLAY_REFUSED;
 case CMD_WAKE:if(!own_veh(c.a))return PLAY_BAD_ARGS;return E::order_wake(c.a)?PLAY_OK:PLAY_REFUSED;
 case CMD_DISBAND:{
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  const int unit=E::Vehs[c.a].unit_id;
  if(!E::order_disband(c.a))return PLAY_REFUSED;
  add(MK_ACTION,"%s disbanded",unit_name(unit));return PLAY_OK;}
 case CMD_SET_HOME:
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  if(!E::order_set_home(c.a))return PLAY_REFUSED;
  add(MK_ACTION,"New home base: %s",E::Vehs[c.a].home_base_id>=0?E::Bases[E::Vehs[c.a].home_base_id].name:"-");return PLAY_OK;
 case CMD_RETURN_HOME:if(!own_veh(c.a))return PLAY_BAD_ARGS;return E::order_return_home(c.a)?PLAY_OK:PLAY_REFUSED;
 case CMD_UPGRADE:{
  if(!own_veh(c.a))return PLAY_BAD_ARGS;
  const int target=c.b>=0?c.b:best_upgrade(me,c.a);
  if(target<0)return PLAY_REFUSED;
  const int credits=E::Factions[me].energy_credits;
  if(!E::order_upgrade(c.a,target))return PLAY_REFUSED;
  add(MK_ACTION,"Upgraded to %s for %d energy",unit_name(target),credits-E::Factions[me].energy_credits);return PLAY_OK;}
 case CMD_ODDS:
  if(!own_veh(c.a) || !E::mapsq(c.b,c.c))return PLAY_BAD_ARGS;
  fill_battle_preview(c.a,c.b,c.c);return battle.odds>0?PLAY_OK:PLAY_REFUSED;
 case CMD_ATTACK:{
  if(!own_veh(c.a) || !E::mapsq(c.b,c.c))return PLAY_BAD_ARGS;
  fill_battle_preview(c.a,c.b,c.c);
  std::unique_ptr<E::BattleResult> r(new E::BattleResult());
  const int outcome=E::engine_attack(c.a,c.b,c.c,r.get());
  fill_battle_result(*r);
  if(outcome==E::BATTLE_INVALID || outcome==E::BATTLE_NO_RULES || outcome==E::BATTLE_NO_TARGET){battle.valid=false;return PLAY_REFUSED;}
  add(MK_COMBAT,"Battle at %d, %d: %s (%d rounds)",c.b,c.c,E::battle_outcome_name(outcome),r->rounds);
  return PLAY_OK;}
 case CMD_PRODUCE:{
  if(!own_base(c.a))return PLAY_BAD_ARGS;
  if(!E::base_queue_valid_item(c.a,c.b))return PLAY_REFUSED;
  E::mod_base_change(c.a,c.b);
  E::set_base(c.a);E::base_compute(1);
  add(MK_BASE,"%s now builds %s",E::Bases[c.a].name,item_name(c.b));return PLAY_OK;}
 case CMD_QUEUE_ADD:
  if(!own_base(c.a))return PLAY_BAD_ARGS;
  if(E::base_queue_append(c.a,c.b)<0)return PLAY_REFUSED;
  add(MK_BASE,"%s queue: %s added",E::Bases[c.a].name,item_name(c.b));return PLAY_OK;
 case CMD_QUEUE_REMOVE:
  if(!own_base(c.a) || c.b<1)return PLAY_BAD_ARGS;
  return E::base_queue_remove(c.a,c.b)<0?PLAY_REFUSED:PLAY_OK;
 case CMD_QUEUE_MOVE:
  if(!own_base(c.a) || c.b<1 || c.c<1)return PLAY_BAD_ARGS;
  return E::base_queue_move(c.a,c.b,c.c)<0?PLAY_REFUSED:PLAY_OK;
 case CMD_HURRY:{
  if(!own_base(c.a))return PLAY_BAD_ARGS;
  const int cost=E::base_hurry_quote(c.a,-1);const int r=E::base_hurry_player(c.a,-1);
  if(r!=E::HURRY_DONE)return PLAY_REFUSED;
  add(MK_BASE,"%s: production hurried for %d energy",E::Bases[c.a].name,cost);return PLAY_OK;}
 case CMD_RESEARCH:{
  const int r=E::research_change(me,c.a,c.c!=0);
  if(r==E::RESEARCH_NEEDS_CONFIRMATION)return PLAY_NEEDS_CONFIRM;
  if(r==E::RESEARCH_INVALID)return PLAY_REFUSED;
  preferred_tech=c.a;
  add(MK_RESEARCH,"Research goal: %s",E::Tech[c.a].name?E::Tech[c.a].name:"?");return PLAY_OK;}
 case CMD_SOCIAL:{
  if(c.a<0 || c.a>=E::MaxSocialCatNum || c.b<0 || c.b>3 || !E::society_avail(c.a,c.b,me))return PLAY_REFUSED;
  const int cost=E::social_change(me,c.a,c.b);
  if(cost<0)return PLAY_REFUSED;
  add(MK_INFO,"Social engineering: %s %s (upheaval %d)",E::SocialField[c.a].field_name?E::SocialField[c.a].field_name:"",
   E::SocialField[c.a].soc_name[c.b]?E::SocialField[c.a].soc_name[c.b]:"",cost);return PLAY_OK;}
 case CMD_DESIGN:{
  if(c.a<0 || c.a>=E::MaxChassisNum || c.b<0 || c.b>=E::MaxWeaponNum || c.c<0 || c.c>=E::MaxArmorNum || c.d<1 || c.d>E::MaxReactorNum)return PLAY_BAD_ARGS;
  const int u=E::create_proto(me,(E::VehChassis)c.a,(E::VehWeapon)c.b,(E::VehArmor)c.c,(E::VehAblFlag)c.e,(E::VehReactor)c.d,(E::VehPlan)plan_for(c.a,c.b,c.c));
  if(u<0)return PLAY_REFUSED;
  add(MK_RESEARCH,"New prototype: %s",unit_name(u));return PLAY_OK;}
 case CMD_DIPLO:{
#if PLAY_HAS_DIPLO
  if(c.a<=0 || c.a>=E::MaxPlayerNum || c.a==me || !E::is_alive(c.a))return PLAY_BAD_ARGS;
  if(!dialog_open || dialog.other!=c.a){ui_diplo::diplo_dialog_open(dialog,me,c.a);dialog_open=true;}
  if(c.b<0){dialog_open=c.b==-1;return PLAY_OK;}  // -1 opens, -2 closes
  const int answer=ui_diplo::diplo_dialog_propose(dialog,c.b);
  if(answer==ui_diplo::DA_INVALID)return PLAY_REFUSED;
  add(MK_INFO,"%s: %s %s",noun(c.a),ui_diplo::diplo_proposal_label(c.b),answer==ui_diplo::DA_ACCEPTED?"accepted":"refused");
  return PLAY_OK;
#else
  return PLAY_UNAVAILABLE;
#endif
 }
 case CMD_END_TURN:return end_turn();
 case CMD_EDIT_UNIT:{
  if(c.a<0 || c.a>=E::MaxPlayerNum || c.b<0 || c.b>=E::MaxProtoNum || !E::mapsq(c.c,c.d))return PLAY_BAD_ARGS;
  const int v=E::veh_init(c.b,c.a,c.c,c.d);
  if(v<0)return PLAY_REFUSED;
  add(MK_INFO,"Editor: %s %s placed at %d, %d",noun(c.a),unit_name(c.b),c.c,c.d);return PLAY_OK;}
 default:return PLAY_BAD_ARGS;
 }
}

/* ---- Session ---- */
Session::Session():impl_(new SessionImpl()){}
Session::~Session(){
 if(E::engine_state()==impl_->state.get()){E::set_event_sink(nullptr);E::set_tech_chooser(nullptr,nullptr);E::engine_bind(nullptr);}
 delete impl_;
}
void Session::bind(){impl_->bind();}
int Session::load_rules(const char* dir,int expansion){
 bind();
 if(E::engine_load_rules(dir,expansion)){impl_->rules_error=E::engine_rules_error();return PLAY_IO;}
 impl_->rules_error.clear();return PLAY_OK;
}
const char* Session::rules_error()const{return impl_->rules_error.c_str();}
int Session::load_game(const uint8_t* bytes,size_t length){
 bind();
 const int r=E::load_daemon_bytes(bytes,length);
 if(r!=E::SAVE_LOAD_VALID){impl_->add(MK_WARNING,"Load failed (status %d)",r);return PLAY_IO;}
 impl_->after_load();
 impl_->add(MK_INFO,"Loaded: %s, turn %d, mission year %d",SessionImpl::noun(impl_->player()),*E::CurrentTurn,*E::CurrentMissionYear);
 return PLAY_OK;
}
bool Session::new_game_available(){return PLAY_HAS_NEWGAME!=0;}
bool Session::diplomacy_available(){return PLAY_HAS_DIPLO!=0;}
int Session::new_game(uint32_t seed,int faction,int difficulty,int map_size){
#if PLAY_HAS_NEWGAME
 bind();
 E::NewGameSettings s;s.seed=seed;s.player_faction=faction;s.difficulty=difficulty;s.map_size=map_size;
 if(E::engine_new_game(s))return PLAY_REFUSED;
 return adopt_new_game();
#else
 (void)seed;(void)faction;(void)difficulty;(void)map_size;return PLAY_UNAVAILABLE;
#endif
}
int Session::adopt_new_game(){
 bind();
 impl_->after_load();
 impl_->add(MK_INFO,"Planetfall: %s",SessionImpl::noun(impl_->player()));
 const int r=impl_->end_turn(); // turn 0 -> the first human turn
 return r;
}
int Session::save_game(std::vector<uint8_t>& out){
 if(!impl_->loaded)return PLAY_NO_GAME;
 bind();out.clear();
 const int r=E::save_daemon_bytes(out);
 if(r!=0 || out.empty())return PLAY_IO;
 return PLAY_OK;
}
int Session::view_bytes(std::vector<uint8_t>& out){
 if(!impl_->loaded)return PLAY_NO_GAME;
 bind();out.clear();
 return E::game_write_bytes(out)==E::SAVE_OK && !out.empty()?PLAY_OK:PLAY_IO;
}
bool Session::loaded()const{return impl_->loaded;}
bool Session::game_over()const{return impl_->over;}
int Session::run(const Command& c){bind();return impl_->run(c);}
const TurnInfo& Session::last_turn()const{return impl_->last;}
size_t Session::message_count()const{return impl_->messages.size();}
const PlayMessage* Session::message(size_t i)const{return i<impl_->messages.size()?&impl_->messages[i]:nullptr;}
void Session::take_messages(){impl_->messages.clear();}
int Session::player()const{return impl_->player();}
int Session::turn()const{return impl_->loaded?*E::CurrentTurn:0;}
int Session::year()const{return impl_->loaded?*E::CurrentMissionYear:0;}
int Session::credits()const{const int p=impl_->player();return p>=0?E::Factions[p].energy_credits:0;}
int Session::vehicle_count()const{return impl_->loaded?*E::VehCount:0;}
int Session::base_count()const{return impl_->loaded?*E::BaseCount:0;}
bool Session::unit(int v,UnitInfo& u)const{
 u=UnitInfo{};if(!impl_->loaded || v<0 || v>=*E::VehCount)return false;
 E::VEH& veh=E::Vehs[v];u.valid=true;u.id=v;u.unit_id=veh.unit_id;u.faction=veh.faction_id;u.x=veh.x;u.y=veh.y;u.damage=veh.damage_taken;
 u.speed=E::veh_speed(v,0);u.moves_left=u.speed-veh.moves_spent;if(u.moves_left<0)u.moves_left=0;u.order=veh.order;u.home_base=veh.home_base_id;
 E::UNIT& p=E::Units[veh.unit_id];u.colony=p.is_colony();u.former=p.is_former();u.combat=p.is_combat_unit();u.own=veh.faction_id==impl_->player();
 snprintf(u.name,sizeof(u.name),"%s",SessionImpl::unit_name(veh.unit_id));return true;
}
int Session::next_unit(int v)const{
 if(!impl_->loaded)return -1;
 const int n=*E::VehCount,me=impl_->player();
 for(int k=1;k<=n;++k){const int i=((v<0?-1:v)+k)%n;if(i<0)continue;const E::VEH& veh=E::Vehs[i];
  if(veh.faction_id==me && veh.order==E::ORDER_NONE && E::veh_speed(i,0)>veh.moves_spent)return i;}
 return -1;
}
int Session::unit_at(int x,int y)const{
 if(!impl_->loaded)return -1;
 for(int i=0;i<*E::VehCount;++i)if(E::Vehs[i].x==x && E::Vehs[i].y==y && E::Vehs[i].faction_id==impl_->player())return i;
 return -1;
}
int Session::base_at(int x,int y)const{return impl_->loaded?E::base_at(x,y):-1;}
bool Session::step_target(int v,int d,int& x,int& y)const{
 if(!impl_->loaded || v<0 || v>=*E::VehCount || d<0 || d>7)return false;
 x=E::wrap(E::Vehs[v].x+E::BaseOffsetX[d]);y=E::Vehs[v].y+E::BaseOffsetY[d];return E::mapsq(x,y)!=nullptr;
}
bool Session::hostile_at(int x,int y)const{return impl_->loaded && E::mapsq(x,y) && veh_hostile_tile(impl_->player(),x,y);}
int Session::upgrade_target(int v)const{
 if(!impl_->own_veh(v))return -1;
 return best_upgrade(impl_->player(),v);
}

bool Session::base_production(int b,BaseProductionModel& m)const{
 const int keep=m.valid && m.base==b?m.selected:0;
 m=BaseProductionModel{};if(!impl_->own_base(b))return false;
 const E::BASE& base=E::Bases[b];m.valid=true;m.base=b;snprintf(m.name,sizeof(m.name),"%s",base.name);
 m.current=base.queue_items[0];snprintf(m.current_name,sizeof(m.current_name),"%s",SessionImpl::item_name(m.current));
 m.minerals=base.minerals_accumulated;m.cost=E::mineral_cost(b,m.current);m.surplus=base.mineral_surplus;
 m.turns=m.surplus>0?(m.cost-m.minerals+m.surplus-1)/m.surplus:0;if(m.turns<0)m.turns=0;
 m.queue_count=E::base_queue_count(b);if(m.queue_count>10)m.queue_count=10;
 for(int q=0;q<m.queue_count;++q){m.queue[q]=base.queue_items[q];snprintf(m.queue_names[q],sizeof(m.queue_names[q]),"%s",SessionImpl::item_name(base.queue_items[q]));}
 m.hurry_cost=E::base_hurry_quote(b,-1);m.credits=E::Factions[base.faction_id].energy_credits;m.hurried=(base.state_flags&E::BSTATE_HURRY_PRODUCTION)!=0;
 const int f=base.faction_id;const int cap=(int)(sizeof(m.items)/sizeof(m.items[0]));
 auto push=[&](int item){if(m.count>=cap || !E::base_queue_valid_item(b,item))return;BaseProductionItem& it=m.items[m.count++];it.id=item;
  snprintf(it.name,sizeof(it.name),"%s",SessionImpl::item_name(item));it.cost=E::mineral_cost(b,item);it.turns=m.surplus>0?(it.cost+m.surplus-1)/m.surplus:0;};
 for(int u=0;u<E::MaxProtoFactionNum;++u)push(u);
 for(int u=f*E::MaxProtoFactionNum;u<(f+1)*E::MaxProtoFactionNum && f>0;++u)push(u);
 for(int i=1;i<=E::SP_ID_Last;++i)push(-i);
 m.selected=keep<m.count?keep:0;
 return true;
}
bool Session::research(ResearchPanelModel& m)const{
 const int keep=m.valid?m.selected:0;m=ResearchPanelModel{};const int me=impl_->player();if(me<=0)return false;
 std::unique_ptr<E::TechChoice> c(new E::TechChoice());
 const int n=E::research_pool(me,-1,c.get());
 const E::Faction& f=E::Factions[me];m.valid=true;m.faction=me;m.current=f.tech_research_id;
 snprintf(m.current_name,sizeof(m.current_name),"%s",m.current>=0 && m.current<E::MaxTechnologyNum && E::Tech[m.current].name?E::Tech[m.current].name:"(none)");
 m.accumulated=f.tech_accumulated;m.cost=E::tech_rate(me);m.labs=f.labs_total;m.known=f.tech_ranking/2;
 static const char* const cats[4]={"Explore","Discover","Build","Conquer"};
 for(int i=0;i<n && m.count<32;++i){const int id=c->ids[i];if(id<0 || id>=E::MaxTechnologyNum)continue;m.ids[m.count]=id;
  snprintf(m.names[m.count],sizeof(m.names[m.count]),"%s",E::Tech[id].name?E::Tech[id].name:"?");
  snprintf(m.categories[m.count],sizeof(m.categories[m.count]),"%s",cats[E::tech_category(id)&3]);++m.count;}
 m.selected=keep<m.count?keep:0;return true;
}
bool Session::social(SocialPanelModel& m)const{
 const int row=m.valid?m.row:0,col=m.valid?m.col:0;m=SocialPanelModel{};const int me=impl_->player();if(me<=0)return false;
 E::Faction& f=E::Factions[me];m.valid=true;m.faction=me;m.credits=f.energy_credits;m.row=row;m.col=col;
 const int pending[4]={f.SE_Politics_pending,f.SE_Economics_pending,f.SE_Values_pending,f.SE_Future_pending};
 for(int c=0;c<4;++c){m.current[c]=pending[c];snprintf(m.categories[c],sizeof(m.categories[c]),"%s",E::SocialField[c].field_name?E::SocialField[c].field_name:"");
  for(int k=0;k<4;++k){snprintf(m.models[c][k],sizeof(m.models[c][k]),"%s",E::SocialField[c].soc_name[k]?E::SocialField[c].soc_name[k]:"");
   m.available[c][k]=E::society_avail(c,k,me)!=0;
   E::CSocialCategory choice;choice.politics=pending[0];choice.economics=pending[1];choice.values=pending[2];choice.future=pending[3];
   choice.models[c]=k;m.cost[c][k]=E::social_upheaval(me,&choice);}}
 return true;
}
bool Session::design(DesignPanelModel& m)const{
 DesignPanelModel old=m;m=DesignPanelModel{};const int me=impl_->player();if(me<=0)return false;
 m.valid=true;m.faction=me;m.last_unit=old.valid?old.last_unit:-1;m.row=old.valid?old.row:0;
 auto add=[&](int row,int id,const char* name){if(m.counts[row]>=32 || !name || !name[0])return;DesignChoice& d=m.choices[row][m.counts[row]++];d.id=id;snprintf(d.name,sizeof(d.name),"%s",name);};
 for(int i=0;i<E::MaxChassisNum;++i)if(E::has_tech(E::Chassis[i].preq_tech,me))add(DESIGN_CHASSIS,i,E::Chassis[i].offsv1_name);
 for(int i=0;i<E::MaxWeaponNum;++i)if(E::has_tech(E::Weapon[i].preq_tech,me))add(DESIGN_WEAPON,i,E::Weapon[i].name);
 for(int i=0;i<E::MaxArmorNum;++i)if(E::has_tech(E::Armor[i].preq_tech,me))add(DESIGN_ARMOR,i,E::Armor[i].name);
 for(int i=1;i<=E::MaxReactorNum;++i)if(E::has_tech(E::Reactor[i-1].preq_tech,me))add(DESIGN_REACTOR,i,E::Reactor[i-1].name);
 add(DESIGN_ABILITY,-1,"None");
 for(int i=0;i<E::MaxAbilityNum;++i)if(E::has_tech(E::Ability[i].preq_tech,me))add(DESIGN_ABILITY,i,E::Ability[i].name);
 for(int r=0;r<DESIGN_ROWS;++r){m.selected[r]=old.valid && old.selected[r]<m.counts[r]?old.selected[r]:0;}
 design_preview(m);return true;
}
void Session::design_preview(DesignPanelModel& m)const{
 m.preview_ok=false;m.preview_name[0]=0;m.preview_cost=0;const int me=impl_->player();if(me<=0 || !m.valid)return;
 int chs,wpn,arm,rec,abl;design_selection(m,chs,wpn,arm,rec,abl);
 if(chs<0 || wpn<0 || arm<0 || rec<1)return;
 char name[256];name[0]=0;
 E::mod_name_proto(name,-1,me,(E::VehChassis)chs,(E::VehWeapon)wpn,(E::VehArmor)arm,(E::VehAblFlag)abl,(E::VehReactor)rec);
 snprintf(m.preview_name,sizeof(m.preview_name),"%.39s",name);
 m.preview_cost=E::mod_proto_cost((E::VehChassis)chs,(E::VehWeapon)wpn,(E::VehArmor)arm,(E::VehAblFlag)abl,(E::VehReactor)rec);
 m.preview_ok=m.preview_cost>0 && name[0];
}
const BattlePanelModel& Session::battle()const{return impl_->battle;}
bool Session::diplomacy(int other,DiploPanelModel& m)const{
 const int keep=m.valid && m.other==other?m.selected:0;m=DiploPanelModel{};
#if PLAY_HAS_DIPLO
 const int me=impl_->player();if(me<=0 || other<=0 || other>=E::MaxPlayerNum || other==me || !E::is_alive(other))return false;
 ui_diplo::DiploDialog d;
 if(impl_->dialog_open && impl_->dialog.other==other)d=impl_->dialog;else ui_diplo::diplo_dialog_open(d,me,other);
 m.valid=true;m.player=me;m.other=other;
 snprintf(m.other_name,sizeof(m.other_name),"%s",E::MFactions[other].formal_name_faction);
 snprintf(m.leader,sizeof(m.leader),"%.23s %.23s",E::MFactions[other].title_leader,E::MFactions[other].name_leader);
 snprintf(m.mood,sizeof(m.mood),"%s",ui_diplo::diplo_mood_name(d.mood));
 const uint32_t s=d.status;
 snprintf(m.status,sizeof(m.status),"%s",(s&E::DIPLO_PACT)?"Pact of Brotherhood":(s&E::DIPLO_TREATY)?"Treaty of Friendship":(s&E::DIPLO_TRUCE)?"Truce":(s&E::DIPLO_VENDETTA)?"Vendetta":"No agreement");
 for(int i=0;i<d.option_count && m.count<16;++i){m.proposals[m.count]=d.options[i];snprintf(m.labels[m.count],sizeof(m.labels[m.count]),"%s",ui_diplo::diplo_proposal_label(d.options[i]));++m.count;}
 m.last_answer=d.last_answer;snprintf(m.reply,sizeof(m.reply),"%s",d.reply);
 m.selected=keep<m.count?keep:0;return true;
#else
 (void)other;(void)keep;return false;
#endif
}
int Session::contacts(int* ids,int max)const{
 const int me=impl_->player();int n=0;if(me<=0)return 0;
 for(int f=1;f<E::MaxPlayerNum && n<max;++f)if(f!=me && E::is_alive(f) && (E::Factions[me].diplo_status[f]&E::DIPLO_COMMLINK))ids[n++]=f;
 return n;
}
uint32_t Session::digest(){
 std::vector<uint8_t> b;if(view_bytes(b)!=PLAY_OK)return 0;
 uint32_t h=2166136261u;for(uint8_t x:b)h=(h^x)*16777619u;return h;
}

Session& session(){static Session s;return s;}

}
}
