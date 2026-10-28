/* SPDX-License-Identifier: GPL-3.0-or-later
 * Headless scripted play through the same command layer the game screen uses
 * (play_session.h). No window, no original assets: the rules directory is the
 * synthetic one test_play_session.py writes.
 *
 *   play_control play RULES_DIR EXPANSION SEED OUT_DIR [TURNS]
 *
 * Starts a new game, moves units, founds a base, sets production, runs End
 * Turn TURNS times (default 20), attacks, changes research, social
 * engineering and a design, hurries production, saves, loads the save into a
 * second session and continues both: the two must stay identical. After every
 * step the view is rebuilt from the engine's SAV bytes with OriginalWorld and
 * checked against the engine, and the panels are rendered. Everything printed
 * depends only on the inputs, so host and big-endian m68k runs must match.
 */
#include "play_session.h"
#include "original_world.h"
#include "ui_basewin.h"
#include "ui_battle.h"
#include "ui_design.h"
#include "ui_diplowin.h"
#include "ui_messages.h"
#include "ui_research.h"
#include "ui_social.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace alpha;
using alpha::play::Command;
using alpha::play::Session;

static int failures=0;
static void check(bool ok,const char* what){if(!ok){++failures;printf("FAIL %s\n",what);}}
static uint32_t fnv(const uint8_t* p,size_t n,uint32_t h=2166136261u){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}

static const unsigned FW=640,FH=480;
static uint8_t frame[FW*FH*3];
static UiTextItem items[256];
static void show_items(const char* panel,size_t n){
 printf("PANEL %s items=%lu frame=%08lx\n",panel,(unsigned long)n,(unsigned long)fnv(frame,sizeof(frame)));
 for(size_t i=0;i<n;++i)printf("  |%s|\n",items[i].text);
}
static UiFrame ui_frame(){memset(frame,0,sizeof(frame));return UiFrame{frame,sizeof(frame),FW,FH};}

static void drain(Session& s,const char* who){
 for(size_t i=0;i<s.message_count();++i){const auto* m=s.message(i);printf("%s MSG t%d k%u %s\n",who,(int)m->turn,(unsigned)m->kind,m->text);}
 s.take_messages();
}
static int run(Session& s,const Command& c,const char* who="A"){
 const int r=s.run(c);
 printf("%s CMD %s %d %d %d %d %d -> %s\n",who,play::command_name(c.kind),c.a,c.b,c.c,c.d,c.e,play::play_status_name(r));
 drain(s,who);return r;
}
/* Rebuild the window's view from the engine bytes and compare. */
static void view_check(Session& s,const char* who){
 std::vector<uint8_t> bytes;const int r=s.view_bytes(bytes);check(r==play::PLAY_OK,"view bytes");
 OriginalWorld w;const auto e=w.load(bytes.data(),bytes.size(),0x12345678u);
 check(e==WorldLoadError::ok,"OriginalWorld load of engine bytes");
 if(e!=WorldLoadError::ok){printf("%s VIEW error=%s\n",who,world_load_error_name(e));return;}
 check((int)w.game().vehicle_count==s.vehicle_count(),"view vehicle count");
 check((int)w.game().base_count==s.base_count(),"view base count");
 int mismatched=0;
 for(int v=0;v<s.vehicle_count();++v){play::UnitInfo u;s.unit(v,u);const auto* o=w.vehicle((size_t)v);if(!o || o->x!=u.x || o->y!=u.y || o->unit_id!=u.unit_id)++mismatched;}
 check(!mismatched,"view vehicle positions");
 printf("%s VIEW bytes=%lu digest=%08lx vehicles=%lu bases=%lu turn=%ld\n",who,(unsigned long)bytes.size(),(unsigned long)fnv(bytes.data(),bytes.size()),
  (unsigned long)w.game().vehicle_count,(unsigned long)w.game().base_count,(long)w.game().preferences.CurrentTurn);
 if(w.game().base_count){BaseWindowModel m;for(size_t b=0;b<w.game().base_count;++b)if(w.base(b)->faction_id==s.player()){
  if(build_base_window(w,b,nullptr,nullptr,s.player(),m)){size_t n=render_base_window(m,ui_frame(),{0,0,560,420},items,256);printf("%s BASEWIN %lu items=%lu frame=%08lx\n",who,(unsigned long)b,(unsigned long)n,(unsigned long)fnv(frame,sizeof(frame)));}
  break;}}
}
static void list_units(Session& s,const char* who){
 for(int v=0;v<s.vehicle_count();++v){play::UnitInfo u;s.unit(v,u);if(!u.own)continue;
  printf("%s UNIT %d %s at %d,%d moves %d/%d order %d%s%s%s\n",who,v,u.name,u.x,u.y,u.moves_left,u.speed,u.order,u.colony?" colony":"",u.former?" former":"",u.combat?" combat":"");}
}
static int find_unit(Session& s,bool colony,bool former,bool combat,int skip=-1){
 for(int v=0;v<s.vehicle_count();++v){play::UnitInfo u;s.unit(v,u);if(!u.own || v==skip)continue;
  if((colony && u.colony) || (former && u.former) || (combat && u.combat && !u.colony && !u.former))return v;}
 return -1;
}
static int own_base(Session& s){std::vector<uint8_t> b;for(int i=0;i<s.base_count();++i){BaseProductionModel m{};if(s.base_production(i,m))return i;}return -1;}

/* Move a unit up to `steps` times, trying the directions in a fixed order. */
static void wander(Session& s,int v,int steps,int start_dir){
 for(int k=0;k<steps;++k){bool moved=false;
  for(int d=0;d<8 && !moved;++d){const int dir=(start_dir+d)&7;int x,y;if(!s.step_target(v,dir,x,y) || s.hostile_at(x,y))continue;
   play::UnitInfo before;s.unit(v,before);if(before.moves_left<=0)return;
   if(run(s,play::command(play::CMD_MOVE,v,dir))==play::PLAY_OK){play::UnitInfo after;s.unit(v,after);moved=after.x!=before.x || after.y!=before.y;}}
  if(!moved)return;}
}

static bool read_rules(Session& s,const char* dir,int exp){
 const int r=s.load_rules(dir,exp);printf("RULES %s\n",r==play::PLAY_OK?"ok":s.rules_error());return r==play::PLAY_OK;
}

static int play_game(const char* rules,int exp,uint32_t seed,const char* out,int turns){
 Session& s=play::session();
 if(!read_rules(s,rules,exp))return 20;
 if(!Session::new_game_available()){puts("NEWGAME unavailable in this build");return 20;}
 const int ng=s.new_game(seed,1,2,2);printf("NEWGAME %s player=%d turn=%d year=%d\n",play::play_status_name(ng),s.player(),s.turn(),s.year());drain(s,"A");
 check(ng==play::PLAY_OK,"new game");if(ng!=play::PLAY_OK)return 20;
 view_check(s,"A");list_units(s,"A");

 // The new game starts with a base per faction. Production: a colony pod
 // now (hurried), a facility queued behind it.
 int base=own_base(s);check(base>=0,"own base");
 printf("BASES %d\n",s.base_count());
 BaseProductionModel prod{};
 if(base>=0 && s.base_production(base,prod)){
  int facility=-1,colony=-1;for(int i=0;i<prod.count;++i){if(prod.items[i].kind==ITEM_FACILITY && facility<0)facility=prod.items[i].id;if(prod.items[i].kind==ITEM_COLONY && colony<0)colony=prod.items[i].id;}
  check(colony>=0,"colony pod buildable");
  check(run(s,play::command(play::CMD_PRODUCE,base,colony>=0?colony:prod.items[0].id))==play::PLAY_OK,"set production");
  if(facility<0)facility=prod.items[0].id;
  check(run(s,play::command(play::CMD_QUEUE_ADD,base,facility))==play::PLAY_OK,"queue facility");
  s.base_production(base,prod);show_items("production",render_base_production(prod,ui_frame(),{0,0,620,420},items,256));
  int key=base_production_key(prod,'2');printf("BASEKEY down -> %d selected=%d\n",key,prod.selected);
  run(s,play::command(play::CMD_HURRY,base));
 }
 int former=find_unit(s,false,true,false);
 if(former>=0){run(s,play::command(play::CMD_TERRAFORM,former,5));}
 int scout=find_unit(s,false,false,true);
 int founded=0;
 for(int t=0;t<turns;++t){
  // A colony pod from production moves two steps and founds a base.
  const int pod=find_unit(s,true,false,false);
  if(pod>=0){wander(s,pod,2,2*t+1);if(run(s,play::command(play::CMD_BUILD_BASE,pod))==play::PLAY_OK)++founded;}
  if(scout>=0){play::UnitInfo u;if(s.unit(scout,u) && u.own && u.combat)wander(s,scout,2,t);else scout=find_unit(s,false,false,true);}
  if(former>=0){play::UnitInfo u;if(s.unit(former,u) && u.own && u.former && !u.order)run(s,play::command(play::CMD_TERRAFORM,former,t&1?0:5));}
  const int r=run(s,play::command(play::CMD_END_TURN));
  const auto& ti=s.last_turn();printf("A TURN status=%d faction=%d turn=%d year=%d events=%d credits=%d units=%d bases=%d\n",ti.status,ti.faction,ti.turn,ti.year,ti.events,s.credits(),s.vehicle_count(),s.base_count());
  check(r==play::PLAY_OK || r==play::PLAY_GAME_OVER,"end turn");if(r==play::PLAY_GAME_OVER)break;
  view_check(s,"A");
 }
 printf("FOUNDED %d\n",founded);check(founded>0,"base founded by a colony pod");
 list_units(s,"A");

 // Attack: the scenario editor places an enemy next to a combat unit.
 int attacker=find_unit(s,false,false,true);
 if(attacker<0 && base>=0){/* build one now */}
 check(attacker>=0,"combat unit for the attack");
 if(attacker>=0){
  play::UnitInfo a;s.unit(attacker,a);int enemy_faction=-1;for(int f=1;f<8;++f)if(f!=s.player()){enemy_faction=f;break;}
  int tx=-1,ty=-1;for(int d=0;d<8 && tx<0;++d){int x,y;if(s.step_target(attacker,d,x,y) && s.unit_at(x,y)<0 && s.base_at(x,y)<0){
    if(run(s,play::command(play::CMD_EDIT_UNIT,enemy_faction,a.unit_id,x,y))==play::PLAY_OK){tx=x;ty=y;}}}
  check(tx>=0,"enemy placed");
  if(tx>=0){
   run(s,play::command(play::CMD_ODDS,attacker,tx,ty));
   show_items("odds",render_battle_panel(s.battle(),ui_frame(),{0,0,520,300},items,256));
   printf("BATTLEKEY enter -> %d\n",battle_panel_key(s.battle(),UIK_ENTER));
   int moved=run(s,play::command(play::CMD_MOVE,attacker,0));(void)moved;
   const int r=run(s,play::command(play::CMD_ATTACK,attacker,tx,ty));check(r==play::PLAY_OK,"attack");
   show_items("battle",render_battle_panel(s.battle(),ui_frame(),{0,0,520,300},items,256));
  }
  view_check(s,"A");
 }

 // Research, social engineering, design, hurry.
 ResearchPanelModel rp{};check(s.research(rp),"research model");
 show_items("research",render_research_panel(rp,ui_frame(),{0,0,560,420},items,256));
 if(rp.count>1){research_panel_key(rp,UIK_DOWN);int r=run(s,play::command(play::CMD_RESEARCH,rp.ids[rp.selected]));
  if(r==play::PLAY_NEEDS_CONFIRM)r=run(s,play::command(play::CMD_RESEARCH,rp.ids[rp.selected],0,1));
  check(r==play::PLAY_OK,"research change");
  s.research(rp);check(rp.current==rp.ids[1] || rp.current==rp.ids[rp.selected],"research goal set");}
 SocialPanelModel sp{};check(s.social(sp),"social model");
 show_items("social",render_social_panel(sp,ui_frame(),{0,0,620,320},items,256));
 for(int c=0;c<4;++c)for(int k=0;k<4;++k)if(sp.available[c][k] && sp.current[c]!=k && sp.cost[c][k]<=sp.credits){sp.row=c;sp.col=k;c=4;break;}
 if(social_panel_key(sp,UIK_ENTER)==UIA_APPLY)run(s,play::command(play::CMD_SOCIAL,sp.row,sp.col));
 DesignPanelModel dp{};check(s.design(dp),"design model");
 dp.row=DESIGN_WEAPON;design_panel_key(dp,UIK_RIGHT);s.design_preview(dp);
 show_items("design",render_design_panel(dp,ui_frame(),{0,0,560,360},items,256));
 if(dp.preview_ok){int c,w,a,r,ab;design_selection(dp,c,w,a,r,ab);run(s,play::command(play::CMD_DESIGN,c,w,a,r,ab));}
 if(base>=0){run(s,play::command(play::CMD_HURRY,base));}
 int ids[8];const int nc=s.contacts(ids,8);printf("CONTACTS %d diplomacy=%d\n",nc,(int)Session::diplomacy_available());
 if(nc && Session::diplomacy_available()){DiploPanelModel dm{};if(s.diplomacy(ids[0],dm)){show_items("diplo",render_diplo_panel(dm,ui_frame(),{0,0,560,400},items,256));
  if(dm.count)run(s,play::command(play::CMD_DIPLO,ids[0],dm.proposals[0]));}}
 run(s,play::command(play::CMD_END_TURN));view_check(s,"A");

 // Save, load into two new sessions and continue: both continuations must be
 // identical. (The unsaved game A continues too; the engine does not save its
 // random number state, so A may differ from them, as in the original.)
 std::vector<uint8_t> saved;check(s.save_game(saved)==play::PLAY_OK,"save");
 char path[512];snprintf(path,sizeof(path),"%s/play.SAV",out);FILE* f=fopen(path,"wb");if(f){fwrite(saved.data(),1,saved.size(),f);fclose(f);}
 printf("SAVE bytes=%lu digest=%08lx\n",(unsigned long)saved.size(),(unsigned long)fnv(saved.data(),saved.size()));
 std::vector<uint8_t> again;{FILE* g=fopen(path,"rb");if(g){fseek(g,0,SEEK_END);long n=ftell(g);rewind(g);again.resize((size_t)n);check(fread(again.data(),1,(size_t)n,g)==(size_t)n,"read save");fclose(g);}}
 check(again==saved,"save file bytes");
 Session b,c;check(read_rules(b,rules,exp),"rules B");check(read_rules(c,rules,exp),"rules C");
 check(b.load_game(again.data(),again.size())==play::PLAY_OK,"load B");drain(b,"B");
 check(c.load_game(again.data(),again.size())==play::PLAY_OK,"load C");drain(c,"C");
 check(b.digest()==s.digest(),"loaded state equals saved state");
 for(int t=0;t<3;++t){run(s,play::command(play::CMD_END_TURN),"A");run(b,play::command(play::CMD_END_TURN),"B");run(c,play::command(play::CMD_END_TURN),"C");
  const uint32_t db=b.digest(),dc=c.digest();printf("CONTINUE %d A=%08lx B=%08lx C=%08lx\n",t,(unsigned long)s.digest(),(unsigned long)db,(unsigned long)dc);check(db==dc,"continued loaded games equal");}
 if(b.base_count()){BaseProductionModel pb{};for(int i=0;i<b.base_count();++i)if(b.base_production(i,pb))break;
  if(pb.valid && pb.count)run(b,play::command(play::CMD_PRODUCE,pb.base,pb.items[pb.count-1].id),"B");}
 run(b,play::command(play::CMD_END_TURN),"B");
 view_check(b,"B");
 std::vector<uint8_t> fin;b.save_game(fin);snprintf(path,sizeof(path),"%s/final.SAV",out);f=fopen(path,"wb");if(f){fwrite(fin.data(),1,fin.size(),f);fclose(f);}
 printf("FINAL turn=%d units=%d bases=%d digest=%08lx failures=%d\n",b.turn(),b.vehicle_count(),b.base_count(),(unsigned long)b.digest(),failures);
 return failures?1:0;
}

int main(int argc,char** argv){
 setvbuf(stdout,nullptr,_IOLBF,BUFSIZ); // progress stays visible in a redirected log on a slow target
 if(argc>=6 && !strcmp(argv[1],"play"))return play_game(argv[2],atoi(argv[3]),(uint32_t)strtoul(argv[4],nullptr,10),argv[5],argc>6?atoi(argv[6]):20);
 if(argc>=2 && !strcmp(argv[1],"parse")){
  for(int i=2;i<argc;++i){Command c;const bool ok=play::parse_command(argv[i],c);printf("PARSE %s -> %d %s %d %d %d %d %d |%s|\n",argv[i],(int)ok,play::command_name(c.kind),c.a,c.b,c.c,c.d,c.e,c.text);}
  return 0;}
 puts("usage: play_control play RULES_DIR EXPANSION SEED OUT_DIR [TURNS] | parse LINE...");return 20;
}
