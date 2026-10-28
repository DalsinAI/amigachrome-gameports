#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Scripted play test of the game screen's command layer (play_session.cpp)
without a window.

Builds play_control.cpp with every engine source, the play command layer,
the panel modules and the OriginalWorld view reader, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian 68040), and plays one
game against a synthetic rules directory (test_engine_wp0b.make_dir, with
the #RULES rows replaced by playable values; no original assets):
new game, move units, found a base, set production and the queue, End Turn
x TURNS, an attack (odds, then the battle), a research change, social
engineering, a new design, hurry, save, load into a second session and
continue both. After every step the view is rebuilt from the engine's own
SAV bytes with OriginalWorld and compared with the engine.
- the game is played twice and must give identical output and saves;
- with --m68k-linux a plain host build plays the same game and its output
  and saves must be byte-identical to the big-endian run.
Writes receipt.json to the output directory.
"""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import test_engine_wp0b

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
TURNS=20
SEED=777

# Playable #RULES rows in alpha.txt order (row shapes of basic_fixture.json):
# values in the range of the original rules, chosen for this test.
PLAYABLE_RULES=[[3],[2],[3,2],[2],[8],[10],[10],[100],[1],[-1],[5],[1],[7],[14],[50],[50],[50],[1,1],[1,1],[1,1],[10],
 [25],[0],[25],[0],[25],[50],[25],[50],[25],[50],[25],[50],[50],[50],[100],[50],[50],[100],[100],[25],[10],
 [50],[2],[10],[20],[5],[8],[20],
 None,None,None,None,None,None,None,None,None,None,  # technology references: keep the fixture's
 [1],[1],[1],[50],[99],[100],[1,2],[2100],[2600],[2500],[1],[10],[6]]

# Playable component and unit tables in the alphax.txt row formats (new
# synthetic names and values for this test; not the original's).
PLAYABLE_SECTIONS={
 'CHASSIS':[f'{n},M1,{n}2,M1,{n}g,M1,{n}g2,M1,{v},{p},{n}L,M1,{n}gL,M1' for n,v,p in [
  ('Foot','1,0,0,0,0,1','None'),('Rover','2,0,0,0,0,2','T1'),('Hover','3,0,0,0,0,3','T5'),('Boat','4,1,0,0,4,4','None'),
  ('Ship','6,1,0,0,8,6','T6'),('Jet','8,2,2,0,0,8','T8'),('Rotor','8,2,1,0,0,8','T9'),('Grav','8,2,0,0,0,10','T10'),('Rocket','12,2,1,1,0,12','T11')]],
 'REACTORS':['Core A,CA,1,None','Core B,CB,2,T12','Core C,CC,3,T13','Core D,CD,4,T14'],
 'WEAPONS':['Hand Arms,Hand,1,0,1,-1,None','Beam,Beam,2,1,2,-1,T1','Impact,Impact,4,0,4,-1,T2','Rapid Beam,Rapid,5,1,5,-1,T3',
  'Rocket Rack,Rack,6,2,6,-1,T4','Chaotic,Chaotic,8,1,8,-1,T15']+[f'Arm{i},Arm{i},{8+2*i},{i%3},{8+2*i},-1,T{10+i}' for i in range(6,15)]+
  ['Mind Blast,Mind,-1,0,10,-1,T25','Buster,Buster,0,0,32,-1,Disable','Settler Kit,Settler,0,8,10,-1,None','Work Kit,Work,0,9,6,-1,None',
   'Ferry Deck,Ferry,0,7,4,-1,None','Haul Kit,Haul,0,10,8,-1,T26','Agent Kit,Agent,0,11,4,-1,T27','Relic Case,Relic,0,12,36,-1,Disable',
   'Payload,Payload,12,2,12,-1,Disable','Quake,Quake,0,2,12,-1,Disable','Spore,Spore,0,2,12,-1,Disable'],
 'DEFENSES':['Bare,Bare,1,0,1,None','Plate,Plate,2,0,2,T2','Mesh,Mesh,3,1,3,T7']+[f'Shell{i},Shell{i},{i+1},{i%3},{i+1},T{12+i}' for i in range(3,9)]+
  ['Mind Ward,Ward,-1,0,10,T25']+[f'Pulse{i},Pulse{i},{3+5*(i>11)},{i%2},{3+5*(i>11)},Disable' for i in range(10,14)],
 'UNITS':['20']+[f'{n},{c},{w},Bare,-1,0,0,{p},-1,0' for n,c,w,p in [
  ('Settler Pod','Foot','Settler','None'),('Work Crew','Foot','Work','None'),('Patrol','Foot','Hand','None'),('Ferry','Boat','Ferry','None'),
  ('Sea Crew','Boat','Work','None'),('Crawler','Foot','Haul','T26'),('Agent','Rover','Agent','T27'),('Relic','Foot','Relic','Disable'),
  ('Worm Pack','Foot','Mind','Disable'),('Sea Beast','Boat','Mind','Disable'),('Swarm','Rotor','Mind','Disable')]]+
  [f'Spare{i},Foot,Hand,Bare,-1,0,0,Disable,-1,0' for i in range(11,20)],
}

def playable_rules(variant):
 text=test_engine_wp0b.rules_text(variant).decode('cp1252')
 m=re.search(r'#RULES\n(.*?)\n#WORLDBUILDER\n',text,re.S);assert m,'no #RULES section'
 rows=m.group(1).split('\n');assert len(rows)==len(PLAYABLE_RULES),(len(rows),len(PLAYABLE_RULES))
 new=[row if v is None else ','.join(map(str,v)) for row,v in zip(rows,PLAYABLE_RULES)]
 for row,v in zip(rows,PLAYABLE_RULES):
  if v is not None:assert len(row.split(','))==len(v),(row,v)
 text=text[:m.start(1)]+'\n'.join(new)+text[m.end(1):]
 for name,rows in PLAYABLE_SECTIONS.items():
  m=re.search('#'+name+r'\n(.*?)\n#',text,re.S);assert m,name
  old=m.group(1).split('\n');assert len(old)==len(rows),(name,len(old),len(rows))
  text=text[:m.start(1)]+'\n'.join(rows)+text[m.end(1):]
 return text.encode('cp1252')

def sources():
 names=['play_session','play_control','ui_panel','ui_research','ui_social','ui_design','ui_battle','ui_diplowin','ui_basewin',
  'map_overlays','original_map','original_game','original_faction','original_world','pcx_image','ui_newgame','ui_diplo']
 return sorted(ENGINE.glob('*.cpp'))+sorted((ENGINE/'stubs').glob('*.cpp'))+[SRC/(n+'.cpp') for n in names if (SRC/(n+'.cpp')).exists()]

def build(cc,flags,exe):
 cmd=[cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',*flags,'-I',str(ENGINE),'-I',str(SRC),*map(str,sources()),'-o',str(exe)]
 subprocess.run(cmd,check=True);return cmd

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--turns',type=int,default=TURNS);ap.add_argument('--seed',type=int,default=SEED)
 a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 rules=out/'rules';test_engine_wp0b.make_dir(rules,'expansion',rules=playable_rules('expansion'))
 exe=out/'play-control'
 if a.m68k_linux:
  cmd=build(a.cc,['-m68040','-static'],exe);runner=['qemu-m68k','-cpu','m68040']
  ref=out/'play-control-host';build('c++',[],ref)
 else:
  cmd=build(a.cc,['-fsanitize=address,undefined','-fno-sanitize-recover=all'],exe);runner=[];ref=None
 def play(binary,pre,tag):
  d=out/tag;d.mkdir()
  r=subprocess.run([*pre,str(binary),'play',str(rules),'1',str(a.seed),str(d),str(a.turns)],capture_output=True,text=True,timeout=3600,env=env)
  (d/'stdout.txt').write_text(r.stdout)
  assert r.returncode==0,(tag,r.returncode,r.stdout[-4000:],r.stderr[-4000:])
  return r.stdout,(d/'play.SAV').read_bytes(),(d/'final.SAV').read_bytes()
 runs=[play(exe,runner,f'run{k}') for k in range(2)]
 assert runs[0]==runs[1],'second run differs'
 text,mid,final=runs[0];lines=text.split('\n')
 turn_lines=[l for l in lines if l.startswith('A TURN ')]
 assert len(turn_lines)>=a.turns,turn_lines
 assert all(' status=0 faction=1 ' in l for l in turn_lines[:a.turns]),turn_lines
 founded=[l for l in lines if l.startswith('A CMD build ') and l.endswith('-> ok')];assert founded,'no base founded'
 produce=[l for l in lines if l.startswith('A CMD produce ') and l.endswith('-> ok')];assert produce,'production not set'
 attack=[l for l in lines if l.startswith('A CMD attack ')];assert attack and attack[0].endswith('-> ok'),attack
 research=[l for l in lines if l.startswith('A CMD research ') and l.endswith('-> ok')];assert research,'research not changed'
 cont=[l for l in lines if l.startswith('CONTINUE ')];assert len(cont)==3 and all(l.split()[2][2:]==l.split()[3][2:] for l in cont),cont
 assert not [l for l in lines if l.startswith('FAIL')],[l for l in lines if l.startswith('FAIL')]
 views=[l for l in lines if ' VIEW ' in l];assert len(views)>=a.turns
 summary=[l for l in lines if l.startswith('FINAL ')][0]
 rec=dict(command=cmd,runner=runner,turns=a.turns,seed=a.seed,summary=summary,turn_lines=turn_lines,attack=attack,research=research,
  founded=founded,produce=produce,continued=cont,views=len(views),
  output_sha256=hashlib.sha256(text.encode()).hexdigest(),save_sha256=hashlib.sha256(mid).hexdigest(),final_sha256=hashlib.sha256(final).hexdigest(),
  binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest())
 if ref:
  htext,hmid,hfinal=play(ref,[],'host')
  assert htext==text,'host output differs from m68k'
  assert hmid==mid and hfinal==final,'host saves differ from m68k'
  rec['host_equal']=True
 (out/'receipt.json').write_text(json.dumps(rec,indent=2)+'\n')
 print(turn_lines[-1]);print(attack[0]);print(summary)
 print(f'{a.turns} End Turns, base, production, attack, research, save/load/continue: deterministic'
  +(' with byte-identical host and m68k output and saves' if ref else ''))
if __name__=='__main__':main()
