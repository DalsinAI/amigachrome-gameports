#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Reuses the independently interpreted complete parent corpus without copying
# it. New boundary expectations derive directly from its intermediate events.
import argparse,hashlib,json,os,shlex,struct,subprocess
from pathlib import Path
from test_units import reference
ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--fixtures',required=True);ap.add_argument('--cc',default=os.environ.get('CXX','c++'));ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False);fixtures=Path(a.fixtures).resolve();src=Path(__file__).resolve().parent
names=['rules_text.cpp','original_map.cpp','original_game.cpp','original_faction.cpp','original_world.cpp','prototype.cpp','custom_prototype_construction.cpp','existing_prototype_reuse.cpp','proposer_record_stages.cpp','prototype_prefix_context.cpp','automatic_prototype_name.cpp','human_component_pruning.cpp','inactive_prototype_allocation.cpp','retirement_history.cpp','prototype_retirement_queue.cpp','unused_prototype_retirement.cpp','unreferenced_prototype_allocation.cpp','discovery_reactor_sequence.cpp','discovery_reactor_suspension_probe.cpp']
cmd=shlex.split(a.cc)+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie'])+[str(src/n) for n in names]+['-o',str(out/'probe')];subprocess.run(cmd,check=True)
records=[];combined=bytearray();cases=json.loads((fixtures/'receipt.json').read_text())['cases']
for c in cases:
 mode=c['mode']
 # Existing failures remain covered by the unchanged parent query suite.
 if c['error']:continue
 originalFacts=(fixtures/f'{mode}.facts').read_bytes();f=struct.unpack_from('<i',originalFacts)[0];events=c['events'];table=reference((fixtures/f'{mode}.txt').read_bytes(),'base')
 for kind in range(3):
  local=kind==0;unknown=kind==2;facts=bytearray(originalFacts);struct.pack_into('<i',facts,40,f if local else (2 if f!=2 else 3));ca=-(100+mode);cb=0x50607080-mode
  facts+=struct.pack('<3i',0 if unknown else 1,ca,cb);fp=out/f'{mode}-{kind}.facts';fp.write_bytes(facts)
  expected=bytearray((fixtures/f'{mode}.expected').read_bytes());pending=local and bool(events)
  if unknown:expected=bytearray(struct.pack('<I',2))
  else:
   if pending:struct.pack_into('<i',expected,8,events[-1][6]);struct.pack_into('<2i',expected,16,ca,cb)
   reactor=events[-1][1] if pending else 0;name=table[f'reactors.{reactor-1}.name'].encode('cp1252') if pending else b'';assert len(name)<128
   expected+=struct.pack('<2i',int(pending),reactor)+name+bytes(128-len(name))
  current=c['currentDecodedFixture'];baseline=fixtures/'30.SC' if current else fixtures/f'{mode}.SC';run=(['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [])+[str(out/'probe'),str(baseline),str(fixtures/f'{mode}.txt'),str(fp),'0',str(fixtures/f'{mode}.SC') if current else '-']
  r=subprocess.run(run,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=60);assert r.returncode==0,(mode,kind,r.returncode,r.stderr);assert r.stdout==expected,(mode,kind,len(r.stdout),len(expected),next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None),r.stdout[:28].hex(),expected[:28].hex())
  combined+=r.stdout;records.append(dict(mode=mode,kind=kind,pending=pending,unknown=unknown,events=len(events)))
(out/'outputs.bin').write_bytes(combined);receipt=dict(cases=records,allPass=True,bigEndian68040=a.m68k_linux,outputSHA256=hashlib.sha256(combined).hexdigest(),binarySHA256=hashlib.sha256((out/'probe').read_bytes()).hexdigest(),command=cmd,fixtures=str(fixtures));(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(records),receipt['outputSHA256'])
