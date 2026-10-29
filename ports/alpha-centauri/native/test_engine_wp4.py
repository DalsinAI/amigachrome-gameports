#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Engine combat (WP4: engine/combat*.cpp) controls.

Builds engine/tests/wp4_test.cpp with every engine source, on the host with
ASan/UBSan or for m68k-linux under qemu (big-endian), and runs it. The test
uses synthetic rules and worlds only (no original assets) and compares the
engine with the existing pure combat modules as differential oracles:
- combat_rounds.cpp/combat_crt.cpp (this directory): every full battle's
  rounds, hits and winner from the same MSVC CRT seed;
- the recovered original battle maths of the prebattle prefix work
  (combat_prototype, terrain_defense, basic_combat, sensor_combat,
  combat_difficulty, native_combat_morale, artillery_combat). They are taken
  from this directory when present, otherwise from --prebattle DIR or, failing
  that, from the feat/alpha-owned-prebattle-prefix branch via git; without
  them those cases are reported as skipped (the receipt says so).

Until WP3 lands, its stub veh_speed/proto_speed return 0, which would zero
every battle's offense (hasty attack). The test therefore builds a copy of
engine/stubs/wp3.cpp in which these two functions are Thinker's
(src/veh.cpp, MIT); when stubs/wp3.cpp no longer exists the real WP3 code is
used unchanged.
"""
import argparse,hashlib,json,os,re,shutil,subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

SRC=Path(__file__).resolve().parent
ENGINE=SRC/'engine'
PREBATTLE_BRANCH='origin/feat/alpha-owned-prebattle-prefix'
PREBATTLE=['artillery_combat','basic_combat','combat_difficulty','combat_prototype',
 'native_combat_morale','sensor_combat','terrain_defense']

SPEED='''int __cdecl proto_speed(int unit_id) { // Thinker src/veh.cpp (test double)
    if (unit_id == BSC_FUNGAL_TOWER) {
        return 0;
    }
    UNIT* u = &Units[unit_id];
    int speed_val = u->speed();
    int triad = u->triad();
    int weapon_id = u->weapon_id;
    if (triad == TRIAD_AIR) {
        speed_val += u->reactor_id * 2;
    }
    if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_SLOW)) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_ANTIGRAV_STRUTS)) {
        speed_val += (triad == TRIAD_AIR) ? u->reactor_id * 2 : 1;
    }
    if (triad == TRIAD_AIR) {
        if (has_abil(unit_id, ABL_FUEL_NANOCELLS)) {
            speed_val += 2;
        }
        if (has_project(FAC_CLOUDBASE_ACADEMY, unit_id / MaxProtoFactionNum)) {
            speed_val += 2;
        }
        if (has_abil(unit_id, ABL_AIR_SUPERIORITY)) {
            speed_val = (speed_val * 4) / 5;
        }
        if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
            speed_val /= 2;
        }
    }
    return clamp(speed_val, 1, 99) * Rules->move_rate_roads;
}

int __cdecl veh_speed(int veh_id, int skip_morale) { // Thinker src/veh.cpp (test double)
    VEH* veh = &Vehs[veh_id];
    UNIT* u = &Units[veh->unit_id];
    if (veh->unit_id == BSC_FUNGAL_TOWER) {
        return 0;
    }
    bool is_native = veh->is_native_unit();
    int speed_val = proto_speed(veh->unit_id);
    int triad = u->triad();
    if (triad == TRIAD_SEA && has_project(FAC_MARITIME_CONTROL_CENTER, veh->faction_id)) {
        speed_val += Rules->move_rate_roads * 2;
    }
    if (!skip_morale && mod_morale_veh(veh_id, true, 0) == MORALE_ELITE
    && ((is_native && conf.native_elite_moves > 0)
    || (!is_native && conf.normal_elite_moves > 0))) {
        speed_val += Rules->move_rate_roads;
    }
    if (veh->damage_taken && triad != TRIAD_AIR) {
        int moves = speed_val / Rules->move_rate_roads;
        int reactor_val = veh->max_hitpoints();
        speed_val = clamp(reactor_val, 1, 99);
        speed_val = (moves * clamp(reactor_val - veh->damage_taken, 0, 9999)
            + speed_val - 1) / speed_val;
        speed_val = clamp(speed_val, (triad == TRIAD_SEA) ? 2 : 1, 999) * Rules->move_rate_roads;
    }
    return min(conf.max_movement_rate, speed_val);
}
'''

def wp3_double(out):
 stub=ENGINE/'stubs'/'wp3.cpp'
 if not stub.exists():return None
 s=stub.read_text()
 for name in ['veh_speed','proto_speed']:
  s,n=re.subn(r'int __cdecl %s\([^\n]*\) \{\n.*?\n\}\n'%name,'',s,flags=re.S);assert n==1,name
 s=s.replace('#include "../engine.h"','#include "combat.h"')
 s=s.replace('namespace engine {\n','namespace engine {\n\n'+SPEED,1)
 p=out/'wp3_with_speed.cpp';p.write_text(s);return p

def prebattle_dir(out,given):
 if all((SRC/f'{n}.cpp').exists() for n in PREBATTLE):return SRC,'tree'
 if given:return Path(given).resolve(),'--prebattle'
 d=out/'prebattle';d.mkdir()
 try:
  top=subprocess.run(['git','-C',str(SRC),'rev-parse','--show-toplevel'],capture_output=True,text=True,check=True).stdout.strip()
  tar=subprocess.run(['git','-C',top,'archive',PREBATTLE_BRANCH+':ports/alpha-centauri/native'],capture_output=True,check=True).stdout
  subprocess.run(['tar','-x','-C',str(d)],input=tar,check=True)
  return d,'git:'+PREBATTLE_BRANCH
 except (subprocess.CalledProcessError,FileNotFoundError):
  return None,'unavailable'

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++')
 ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--prebattle')
 ap.add_argument('--require-prebattle',action='store_true');a=ap.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);objdir=out/'obj';objdir.mkdir()
 base=['-std=c++17','-O1','-g','-ffunction-sections','-fdata-sections'];runner=[];link=['-Wl,--gc-sections']
 if a.m68k_linux:base+=['-m68040'];link+=['-static'];runner=['qemu-m68k','-cpu','m68040']
 else:base+=['-fsanitize=address,undefined','-fno-sanitize-recover=all'];link+=['-fsanitize=address,undefined']
 strict=['-Wall','-Wextra','-Werror']
 double=wp3_double(out)
 engine=sorted(ENGINE.glob('*.cpp'))+[p for p in sorted((ENGINE/'stubs').glob('*.cpp')) if not (double and p.name=='wp3.cpp')]
 if double:engine.append(double)
 engine+=[ENGINE/'tests'/'wp4_test.cpp']
 pdir,psrc=prebattle_dir(out,a.prebattle)
 if a.require_prebattle and not pdir:raise SystemExit('prebattle oracles unavailable')
 oracle_inc=['-I',str(ENGINE/'tests')]+(['-I',str(pdir)] if pdir else [])+['-I',str(SRC)]
 oracle=[(ENGINE/'tests'/'wp4_oracles.cpp',oracle_inc+(['-DWP4_PREBATTLE_ORACLES'] if pdir else [])),
  (SRC/'combat_crt.cpp',['-I',str(SRC)]),(SRC/'combat_rounds.cpp',['-I',str(SRC)])]
 if pdir:oracle+=[(pdir/f'{n}.cpp',['-I',str(pdir),'-I',str(SRC)]) for n in PREBATTLE]
 jobs=[(p,[*base,*strict,'-I',str(ENGINE)]) for p in engine]+[(p,[*base,*f]) for p,f in oracle]
 def compile(job):
  i,(p,flags)=job;o=objdir/f'{i:03d}-{p.stem}.o'
  r=subprocess.run([a.cc,*flags,'-c',str(p),'-o',str(o)],capture_output=True,text=True)
  if r.returncode:raise SystemExit(f'compile {p}:\n{r.stderr}')
  return str(o)
 with ThreadPoolExecutor(os.cpu_count() or 2) as ex:objs=list(ex.map(compile,enumerate(jobs)))
 exe=out/'wp4-test';subprocess.run([a.cc,*base,*objs,*link,'-o',str(exe)],check=True)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 r=subprocess.run([*runner,str(exe),'unit'],capture_output=True,text=True,timeout=1200,env=env)
 print(r.stdout.strip());
 if r.returncode:print(r.stderr[-6000:]);raise SystemExit(f'wp4 test failed ({r.returncode})')
 (out/'receipt.json').write_text(json.dumps(dict(runner=runner,cc=a.cc,result=r.stdout.strip(),
  prebattle_oracles=psrc,wp3_speed_double=bool(double),
  binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n')
 print('prebattle oracles:',psrc)
if __name__=='__main__':main()
