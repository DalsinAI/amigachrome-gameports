#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write engine/save_fields.inc: explicit little-endian wire visitors.

Parses the record structs in the engine headers (which keep Thinker's field
order and types) and emits one io_fields() visitor per wire record plus its
packed wire size. The original executable stored these records packed
(#pragma pack(1)), so the wire layout is exactly the field order with no
padding. Expected sizes come from Thinker's static_asserts.

Usage: gen_save_fields.py [--check]   (--check fails if the .inc is stale)
"""
import pathlib,re,sys

ENGINE=pathlib.Path(__file__).resolve().parent.parent
HEADERS=['engine_types.h','engine_data.h','engine_base.h','engine_veh.h','engine_win.h','game_state.h']
# record -> expected packed size (Thinker static_asserts / savegame.cpp constants)
RECORDS={'Landmark':0x28,'Continent':0x1C,'Goal':0x10,'MAP':44,'MFaction':1436,'Faction':8396,
         'UNIT':52,'VEH':52,'BASE':308,'GamePrefs':0x398,'MonumentEvent':0x38,'Monument':0x4F4,
         'ReplayEvent':8,'MapMeta':0xAA4,'MapWindow':24}
# fields shorter in pre-v11 (legacy) layouts: record.field -> legacy element count
LEGACY={('Faction','facility_announced'):8,('Faction','secret_project_intel'):5,
        ('BASE','facilities_built'):8,('GamePrefs','SecretProjects'):33}
LEGACY_SIZE={'Faction':0x20BC,'BASE':0x130,'GamePrefs':0x31C}
WIDTH={'int8_t':1,'uint8_t':1,'char':1,'int16_t':2,'uint16_t':2,'int32_t':4,'uint32_t':4,'int':4}

def parse():
    src='\n'.join((ENGINE/h).read_text() for h in HEADERS)
    src=re.sub(r'/\*.*?\*/','',src,flags=re.S)
    src=re.sub(r'//[^\n]*','',src)
    structs={}
    for m in re.finditer(r'\bstruct\s+(\w+)\s*(?::\s*(\w+)\s*)?\{',src):
        name=m.group(1)
        if name not in RECORDS:continue
        i=m.end();depth=1;j=i
        while depth:
            depth+=src[j]=='{';depth-=src[j]=='}';j+=1
        body=src[i:j-1]
        # drop member function bodies
        out='';depth=0
        for ch in body:
            if ch=='{':depth+=1;continue
            if ch=='}':depth-=1;out+=';' if depth==0 else '';continue
            if depth==0:out+=ch
        fields=[]
        for stmt in out.split(';'):
            stmt=' '.join(stmt.split())
            if not stmt or '(' in stmt:continue
            fm=re.match(r'^(?:const\s+)?(\w+)\s+(\w+)((?:\[\w+\])*)$',stmt)
            if not fm:raise SystemExit(f'cannot parse {name}: {stmt!r}')
            dims=re.findall(r'\[(\w+)\]',fm.group(3))
            fields.append((fm.group(1),fm.group(2),dims))
        structs[name]=fields
    return structs

CONST={'MaxPlayerNum':8}
def dim(d):
    return int(d,0) if re.match(r'^(0x)?[0-9a-fA-F]+$',d) else CONST[d]

def size_of(structs,typ):
    if typ in WIDTH:return WIDTH[typ]
    return sum(size_of(structs,t)*prod(d) for t,_,d in structs[typ])
def prod(dims):
    n=1
    for d in dims:n*=dim(d)
    return n

def legacy_size(structs,rec):
    total=0
    for t,n,d in structs[rec]:
        count=LEGACY.get((rec,n))
        total+=size_of(structs,t)*(count if count is not None else prod(d))
    return total

def emit(structs):
    out=['/* SPDX-License-Identifier: GPL-3.0-or-later',
         ' * Written by engine/tools/gen_save_fields.py from the engine headers.',
         ' * Do not edit. Field order and types are Thinker Mod\'s packed records',
         ' * (dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see engine/THINKER-LICENSE).',
         ' * Included by savegame.cpp inside namespace alpha::engine.',
         ' */','']
    for rec,expected in RECORDS.items():
        got=size_of(structs,rec)
        if got!=expected:raise SystemExit(f'{rec}: wire size {got:#x} != expected {expected:#x}')
        out.append(f'const size_t WireSize_{rec} = {got:#x};')
        if rec in LEGACY_SIZE:
            ls=legacy_size(structs,rec)
            if ls!=LEGACY_SIZE[rec]:raise SystemExit(f'{rec}: legacy size {ls:#x} != {LEGACY_SIZE[rec]:#x}')
            out.append(f'const size_t WireSizeLegacy_{rec} = {ls:#x};')
    out.append('')
    for rec in RECORDS:
        has_legacy=any(r==rec for r,_ in LEGACY)
        arg='legacy' if has_legacy else 'UNUSED(legacy)'
        out.append(f'template <class F> void io_fields(F& f, {rec}& s, bool {arg}) {{')
        for t,n,d in structs[rec]:
            if (rec,n) in LEGACY:
                out.append(f'    if (legacy) {{ io_prefix(f, s.{n}, {LEGACY[(rec,n)]}); }} else {{ io(f, s.{n}); }}')
            else:
                out.append(f'    io(f, s.{n});')
        out.append('}')
        out.append('')
    return '\n'.join(out)

def main():
    text=emit(parse())
    path=ENGINE/'save_fields.inc'
    if '--check' in sys.argv:
        if not path.exists() or path.read_text()!=text:
            raise SystemExit('save_fields.inc is stale; run engine/tools/gen_save_fields.py')
        print('save_fields.inc up to date');return
    path.write_text(text)

if __name__=='__main__':main()
