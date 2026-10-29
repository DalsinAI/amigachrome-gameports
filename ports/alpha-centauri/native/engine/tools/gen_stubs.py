#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write engine/stubs/wpN.cpp placeholders from the sections of engine_wp.h.

Run once by WP0. Packages delete their stub file when they land; do not
rerun this for stubs for packages that already landed.
"""
import pathlib,re,sys

ENGINE=pathlib.Path(__file__).resolve().parent.parent
TITLES={1:'turn scheduler and End Turn',2:'base upkeep, production, growth, riots',
        3:'unit orders, movement, terraforming',4:'combat',5:'research, social engineering, unit design',
        6:'AI',7:'new game and world generation',8:'diplomacy, council, victory, score'}
# Neutral placeholder behaviour where the original has an obvious no-effect value.
BODIES={
 'veh_kill_lift':'    not_implemented("veh_kill_lift");\n    return veh_lift(veh_id);',
 'psi_factor':'    not_implemented("psi_factor");\n    return value;',
 'game_year':'    return Rules->normal_start_year + turn; // Thinker game.cpp, exact',
 'base_compute':'    not_implemented("base_compute");\n    if (update_prev || *ComputeBaseID != *CurrentBaseID) {\n        *ComputeBaseID = *CurrentBaseID;\n    }',
 'mod_cost_factor':'    not_implemented("mod_cost_factor");\n    return 10;',
 'tech_level':'    not_implemented("tech_level");\n    return lvl;',
}

def split_params(params):
    params=params.strip()
    if params in ('','void'):return []
    out=[];depth=0;cur=''
    for ch in params:
        if ch==',' and depth==0:out.append(cur.strip());cur='';continue
        depth+=ch in '(';depth-=ch in ')';cur+=ch
    out.append(cur.strip());return out

def main():
    text=(ENGINE/'engine_wp.h').read_text()
    sections=re.split(r'/\* ---- WP(\d):',text)[1:]
    for i in range(0,len(sections),2):
        wp=int(sections[i]);body=sections[i+1]
        body=re.sub(r'//[^\n]*','',body)
        decls=re.findall(r'^([A-Za-z][^;]*?)\(([^;]*?)\);',body,re.M|re.S)
        out=[f'''/* SPDX-License-Identifier: GPL-3.0-or-later
 * Placeholder entry points for WP{wp} ({TITLES[wp]}).
 * Written by engine/tools/gen_stubs.py from engine_wp.h. Each function raises
 * EV_NOT_IMPLEMENTED and returns a neutral value. WP{wp} deletes this file
 * when it lands its real implementation.
 */
#include "../engine.h"

namespace alpha {{
namespace engine {{
''']
        for head,params in decls:
            head=' '.join(head.split())
            m=re.match(r'(.*?)(\w+)$',head);ret,name=m.group(1).strip(),m.group(2)
            ps=split_params(' '.join(params.split()))
            named=[]
            for p in ps:
                pm=re.match(r'(.*?)(\w+)$',p)
                named.append((pm.group(1).strip(),pm.group(2)))
            used=BODIES.get(name,'')
            sig=', '.join(f'{t} {n}' if n in used else f'{t} UNUSED({n})' for t,n in named)
            plain_ret=ret.replace('__cdecl','').strip()
            if name in BODIES:b=BODIES[name]
            elif plain_ret=='void':b=f'    not_implemented("{name}");'
            else:b=f'    not_implemented("{name}");\n    return 0;'
            out.append(f'\n{ret} {name}({sig}) {{\n{b}\n}}\n')
        out.append('\n} // namespace engine\n} // namespace alpha\n')
        (ENGINE/'stubs'/f'wp{wp}.cpp').write_text(''.join(out))

if __name__=='__main__':main()
