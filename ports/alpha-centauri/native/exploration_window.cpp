/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native interactive movement slice. Placeholder terrain marks; no original art.
 */
#include "exploration.h"
#include "colony_scenario.h"
#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct IntuitionBase* IntuitionBase;
struct GfxBase* GfxBase;
static alpha::Rules technologies;
static alpha::UnitDefinitions definitions;
static alpha::BasicRules basic;
static alpha::WorldRules world;
static alpha::MovementTile tiles[96];
static uint8_t moisture[96];
static alpha::ResourceInfo resources;
static alpha::EarlyColony colony{4,4,1,0,1u<<6,false};
static alpha::ColonyMap colony_map;
static bool worker_mode=false;
static size_t owned_base=34; // parity index of (4,4) on width16

static alpha::ExplorationUnit unit;
static uint32_t turn;
static char status[128]="Click an adjacent land tile or use keypad digits. N: next movement turn.";
static void text(struct RastPort* rp,int x,int y,const char* s){Move(rp,x,y);Text(rp,(STRPTR)s,(ULONG)strlen(s));}
static char terrain(const alpha::MovementTile& t){
  if(t.altitude<0x60)return '~';
  if(t.items&8)return 'M';
  if(t.items&4)return 'r';
  if(t.items&0x20)return 'X';
  if(t.items&0x200000)return 'F';
  return t.rockiness==0x80?'^':'.';
}
static void draw(struct Window* window){
  auto* rp=window->RPort;SetAPen(rp,0);RectFill(rp,window->BorderLeft,window->BorderTop,window->Width-window->BorderRight-1,window->Height-window->BorderBottom-1);
  SetAPen(rp,1);SetBPen(rp,0);SetDrMd(rp,JAM2);
  text(rp,15,35,"ALPHA: EARLY COLONY SLICE - not a complete game");
  char line[160];snprintf(line,sizeof(line),"%s | Turn %lu | Position %ld,%ld | Moves %lu/%lu",unit.prototype<definitions.unit_count?definitions.units[unit.prototype].name:"Unit",(unsigned long)turn,(long)unit.x,(long)unit.y,(unsigned long)unit.budget.spent,(unsigned long)unit.budget.speed);text(rp,15,52,line);
  for(int y=0;y<12;++y)for(int x=y&1;x<16;x+=2){
    int px=36+x*32,py=83+y*23;
    Move(rp,px,py-11);Draw(rp,px+29,py);Draw(rp,px,py+11);Draw(rp,px-29,py);Draw(rp,px,py-11);
    char mark[2]={terrain(tiles[(x+16*y)/2]),0};
    if(!colony.removed && x==colony.x && y==colony.y)mark[0]='B';
    else for(unsigned slot=0;slot<20;++slot)if(colony.workers&(1u<<slot)){size_t index;
      if(alpha::colony_work_index(colony_map,colony,slot,index)==alpha::LogicError::ok && index==(size_t)(x+16*y)/2)mark[0]='w';}
    text(rp,px-3,py+3,mark);
    if(unit.x==x && unit.y==y){Move(rp,px-10,py-7);Draw(rp,px+10,py+7);Move(rp,px+10,py-7);Draw(rp,px-10,py+7);}
  }
  alpha::ColonyYield yield;
  if(!colony.removed && alpha::early_colony_yields(colony_map,colony,resources,basic,yield)==alpha::LogicError::ok){
    alpha::HumanCostFactors costs;alpha::calculate_human_cost_factors(basic,2,0,0,costs);
    snprintf(line,sizeof(line),"Landing Site: pop %ld | Food %ld/%ld | Intake %ld | Use %ld | Surplus %ld",(long)colony.population,(long)colony.food,(long)((colony.population+1)*costs.nutrient),(long)yield.nutrients.intake,(long)yield.nutrients.consumption,(long)yield.nutrients.surplus);text(rp,15,370,line);
    snprintf(line,sizeof(line),"Tile yields: minerals %ld energy %ld (production/research not implemented)",(long)yield.totals.minerals,(long)yield.totals.energy);text(rp,15,388,line);
  }else text(rp,15,370,"Landing Site lost to starvation.");
  text(rp,15,406,worker_mode?"WORKER MODE: click land in base radius to assign/remove a citizen.":"SCOUT MODE: click adjacent land, or use keypad digits to move.");
  text(rp,15,424,"B=base w=worker .=plain ^=rock F=forest X=fungus r=road M=mag ~=sea");
  text(rp,15,442,"7 NW 8 N 9 NE | 4 W 6 E | 1 SW 2 S 3 SE | W workers | N turn | Q quit");
  text(rp,15,460,status);
  text(rp,15,478,"Neutral colony scenario: food/growth only; no AI, combat or saves.");
  text(rp,15,496,"No production, support, riots or specialist output; no full campaign.");
}
static void step(int direction,const alpha::MovementMap& map,const alpha::MovementContext& context){
  alpha::StepResult result;
  auto error=alpha::exploration_step(map,definitions,context,direction,unit,result,colony.removed?nullptr:&owned_base,colony.removed?0:1);
  const char* message="Invalid scenario state";
  if(error==alpha::LogicError::ok){
    switch(result.outcome){
      case alpha::StepOutcome::moved:message="Moved";break;
      case alpha::StepOutcome::fractional_failure:message="Fractional move failed; turn spent";break;
      case alpha::StepOutcome::exhausted:message="No movement left; press N";break;
      case alpha::StepOutcome::blocked:message="Ocean or map edge blocks this land unit";break;
    }
    snprintf(status,sizeof(status),"%s (price %lu)",message,(unsigned long)result.cost);
  } else snprintf(status,sizeof(status),"%s",message);
  printf("STEP direction=%d outcome=%d error=%d x=%ld y=%ld spent=%lu seed=%08lx\n",direction,error==alpha::LogicError::ok?(int)result.outcome:-1,(int)error,(long)unit.x,(long)unit.y,(unsigned long)unit.budget.spent,(unsigned long)unit.budget.random_seed);fflush(stdout);
}
int main(int argc,char** argv){
  if(argc!=3 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))){fprintf(stderr,"Usage: alpha-movement base|expansion alpha.txt|alphax.txt\n");return 20;}
  FILE* file=fopen(argv[2],"rb");if(!file)return 20;
  if(fseek(file,0,SEEK_END)){fclose(file);return 20;}long size=ftell(file);rewind(file);
  if(size<=0 || size>1048576){fclose(file);return 20;}
  auto* data=(uint8_t*)malloc((size_t)size);if(!data){fclose(file);return 20;}
  bool read=fread(data,1,(size_t)size,file)==(size_t)size;fclose(file);
  auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;
  bool valid=read && alpha::read_technologies(data,(size_t)size,technologies)==alpha::Error::ok &&
    alpha::read_basic_rules(data,(size_t)size,technologies,variant,basic,world)==alpha::Error::ok &&
    alpha::read_unit_definitions(data,(size_t)size,technologies,variant,definitions)==alpha::Error::ok &&
    alpha::read_resource_info(data,(size_t)size,resources)==alpha::Error::ok;
  free(data);if(!valid){fprintf(stderr,"Rules failed validation\n");return 20;}
  alpha::PredefinedPrototype scout;
  if(alpha::derive_predefined_prototype(definitions,2,basic.move_rate_roads,scout)!=alpha::LogicError::ok)return 20;
  unit={2,4,4,{scout.speed_points,0,0,0x12345678u}};
  for(int y=0;y<12;++y)for(int x=y&1;x<16;x+=2){
    moisture[(x+16*y)/2]=2;auto& tile=tiles[(x+16*y)/2];tile={0,(uint8_t)(x>=14?0x40:0x60),0};
    if(y==4 && x<12)tile.items=4;
    else if(y==6 && x<12)tile.items=8;
    else if(x==7 || x==8)tile.items=0x20;
    else if(x==10 || x==11)tile.items=0x200000;
    else if(y==2)tile.rockiness=0x80;
  }
  tiles[owned_base].items|=1u;
  alpha::MovementMap map{{16,12,false},tiles,96};colony_map={map,moisture,96};alpha::MovementContext context{1,0,basic.move_rate_roads,false,false};
  IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);
  GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);
  if(!IntuitionBase || !GfxBase){if(GfxBase)CloseLibrary((struct Library*)GfxBase);if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);return 20;}
  struct Window* window=OpenWindowTags(NULL,WA_Title,(ULONG)"Alpha native early colony prototype",WA_Width,610,WA_Height,520,
    WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,
    WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
  if(!window){CloseLibrary((struct Library*)GfxBase);CloseLibrary((struct Library*)IntuitionBase);return 20;}
  printf("ALPHA WINDOW variant=%s unit=%s speed=%lu\n",argv[1],scout.name,(unsigned long)scout.speed_points);fflush(stdout);draw(window);
  bool quit=false;
  while(!quit){WaitPort(window->UserPort);struct IntuiMessage* message;
    while((message=(struct IntuiMessage*)GetMsg(window->UserPort))){
      ULONG kind=message->Class;UWORD code=message->Code;WORD mx=message->MouseX,my=message->MouseY;ReplyMsg((struct Message*)message);
      if(kind==IDCMP_CLOSEWINDOW)quit=true;
      else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(window);draw(window);EndRefresh(window,TRUE);}
      else if(kind==IDCMP_VANILLAKEY){
        int direction=-1;switch(code){case '9':direction=0;break;case '6':direction=1;break;case '3':direction=2;break;case '2':direction=3;break;case '1':direction=4;break;case '4':direction=5;break;case '7':direction=6;break;case '8':direction=7;break;case 'q':case 'Q':quit=true;break;case 'n':case 'N':
          if(!colony.removed && turn!=0xffffffffu){auto updated=colony;alpha::ColonyYield yield;
            if(alpha::early_colony_food_turn(colony_map,resources,basic,updated,yield)==alpha::LogicError::ok &&
              alpha::exploration_next_movement_turn(unit,turn)==alpha::LogicError::ok){colony=updated;
              if(colony.removed)tiles[owned_base].items&=~1u;
              snprintf(status,sizeof(status),"Turn %lu; population %ld, food %ld",(unsigned long)turn,(long)colony.population,(long)colony.food);
              printf("COLONY turn=%lu pop=%ld food=%ld workers=%05lx removed=%d\n",(unsigned long)turn,(long)colony.population,(long)colony.food,(unsigned long)colony.workers,colony.removed);fflush(stdout);
            }else snprintf(status,sizeof(status),"Colony state failed validation; turn not advanced");
          }break;case 'w':case 'W':worker_mode=!worker_mode;break;}
        if(direction>=0)step(direction,map,context);
        draw(window);
      } else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
        bool selected=false;
        for(int y=0;y<12 && !selected;++y)for(int x=y&1;x<16 && !selected;x+=2)if(abs(mx-(36+x*32))*11+abs(my-(83+y*23))*29<=319){
          selected=true;
          if(worker_mode){bool found=false;
            for(unsigned slot=0;slot<20;++slot){size_t index;
              if(alpha::colony_work_index(colony_map,colony,slot,index)==alpha::LogicError::ok && index==(size_t)(x+16*y)/2){
                found=alpha::early_colony_toggle_worker(colony_map,resources,basic,slot,colony)==alpha::LogicError::ok;break;}}
            snprintf(status,sizeof(status),found?"Worker allocation changed":"Cannot assign: population, terrain or base radius limit");
            printf("WORKERS pop=%ld food=%ld mask=%05lx accepted=%d\n",(long)colony.population,(long)colony.food,(unsigned long)colony.workers,found);fflush(stdout);draw(window);continue;
          }
          const int dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};bool found=false;
          for(int d=0;d<8;++d){int32_t tx;if(alpha::movement_wrap_x(map.geometry,unit.x+dx[d],tx)==alpha::LogicError::ok && tx==x && unit.y+dy[d]==y){step(d,map,context);found=true;break;}}
          if(!found)snprintf(status,sizeof(status),"Choose a neighboring tile; no automatic pathfinding yet");
          draw(window);
        }
      }
    }
  }
  CloseWindow(window);CloseLibrary((struct Library*)GfxBase);CloseLibrary((struct Library*)IntuitionBase);
  printf("ALPHA EXIT turn=%lu x=%ld y=%ld spent=%lu\n",(unsigned long)turn,(long)unit.x,(long)unit.y,(unsigned long)unit.budget.spent);return 0;
}
