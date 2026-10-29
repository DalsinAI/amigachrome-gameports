#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original ordinal facilities/projects; synthetic tables only by default."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent

def tables(variant):
 slots,project=(72,39) if variant=='base' else (134,70)
 tech=[[f'Tech{i}',f'T{i}','0','1','2','3','None','None','1'] for i in range(89)]
 rows=[]
 for i in range(1,slots):
  row=[f'Facility{i}',str(i-3),str(i%7-2),f'T{i%89}',('None' if i%3==0 else 'Disable' if i%3==1 else 'T7'),f'Effect{i}']
  if i>=project:row += ['-2','-1','0','1','2']
  rows.append(row)
 return tech,rows

def material(tech,rows,tail='#NEXT'):
 return ('#TECHNOLOGY\n'+'\n'.join(','.join(r) for r in tech)+'\n#FACILITIES\n'+'\n'.join(','.join(r)+',' for r in rows)+'\n'+tail+'\n').encode('latin1')

def sections(data):
 result={};name=None
 for line in data.decode('latin1').splitlines():
  line=line.strip()
  if line.startswith(';'):continue
  if name!='facilities':line=line.split(';',1)[0].strip()
  if not line:continue
  if line.startswith('#'):name=line[1:].lower();result[name]=[];continue
  if name:
   row=[s.strip() for s in line.split(',')]
   if row[-1]=='':row.pop()
   result[name].append(row)
 return result

def reference(data,variant):
 t=sections(data);keys={r[1].lower():i for i,r in enumerate(t['technology'])};keys.update(none=-1,disable=-2);slots,repeat,project=(72,34,39) if variant=='base' else (134,65,70);rows=t['facilities'];assert len(rows)==slots-1
 output=bytearray(struct.pack('<4I',slots,repeat,project,slots-project)+bytes(360))
 def number(s):return int(s[2:],{'b':2,'x':16,'d':10}[s[1].lower()]) if len(s)>2 and s[0]=='0' and s[1].lower() in 'bxd' else int(s)
 for i,row in enumerate(rows,1):
  assert len(row)>=(11 if i>=project else 6)
  output += row[0].encode('latin1').ljust(128,b'\0')+row[5].encode('latin1').ljust(192,b'\0')
  output += struct.pack('<10i',number(row[1]),number(row[2]),keys[row[3].lower()],keys[row[4].lower()],-2,*([number(x) for x in row[6:11]] if i>=project else [0]*5))
 return bytes(output)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',action='append',default=[],help='Private optional PROFILE:PATH');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
 exe=out/'facility-probe';cmd=[a.cc,*flags,str(SRC/'rules_text.cpp'),str(SRC/'facility_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);cases=[]
 def check(name,data,variant,valid=True):
  path=out/(str(len(cases))+'.txt');path.write_bytes(data);r=subprocess.run([*runner,str(exe),variant,str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  if valid:
   wanted=reference(data,variant);assert r.returncode==0 and r.stdout==wanted,(name,r.returncode,r.stderr,len(r.stdout),len(wanted));(out/(str(len(cases))+'.bin')).write_bytes(r.stdout)
  else:assert r.returncode==20 and not r.stdout and b'ERROR' in r.stderr,(name,r.returncode,r.stdout[:100],r.stderr)
  cases.append(dict(name=name,variant=variant,valid=valid,passed=True,outputSHA256=hashlib.sha256(r.stdout).hexdigest()))
 for variant in ['base','expansion']:
  tech,rows=tables(variant);data=material(tech,rows);check(variant,data,variant);check(variant+'-crlf',data.replace(b'\n',b'\r\n'),variant);check(variant+'-comments',data.replace(b'#FACILITIES',b'#facilities\n;comment\n'),variant);check(variant+'-no-terminal-commas',data.replace(b',\n',b'\n'),variant);check(variant+'-wrong-profile',data,'base' if variant=='expansion' else 'expansion',False)
  for i in range(len(rows)):check(variant+'-truncated'+str(i),material(tech,rows[:i]),variant,False)
  check(variant+'-extra',material(tech,rows+[rows[-1]]),variant,False);check(variant+'-missing-section',data.replace(b'#FACILITIES',b'#OTHER'),variant,False)
  for name,index,column,value,valid in [('unknown-prereq',0,3,'BADTECH',False),('unknown-configured-free',0,4,'BADTECH',False),('empty-name',0,0,'',True),('empty-effect',0,5,'',True),('literal-semicolon',0,5,'one;two',True),('long-name-bound',0,0,'a'*127,True),('long-name-reject',0,0,'a'*128,False),('long-effect-bound',0,5,'a'*191,True),('long-effect-reject',0,5,'a'*192,False),('cost-min',0,1,'-2147483648',True),('cost-max',0,1,'2147483647',True),('maint-min',0,2,'-2147483648',True),('cost-overflow',0,1,'2147483648',False),('ai-extreme',len(rows)-1,6,'-2147483648',True),('ai-overflow',len(rows)-1,6,'2147483648',False)]:
   edited=[r.copy() for r in rows];edited[index][column]=value;check(variant+'-'+name,material(tech,edited),variant,valid)
  check(variant+'-ordinary-extra-token',data.replace(b'Effect1,\n',b'Effect1,garbage,\n'),variant,True);check(variant+'-project-missing-priority',data.replace(b',-2,-1,0,1,2,\n',b',-2,-1,0,1,\n'),variant,False);check(variant+'-double-terminal-comma',data.replace(b'Effect1,\n',b'Effect1,,\n'),variant,True)
 for spec in a.original:
  variant,file=spec.split(':',1);p=Path(file);before=p.read_bytes();check('original-'+p.name,before,variant);assert p.read_bytes()==before
 receipt=dict(command=cmd,cases=cases,accepted=sum(r['valid'] for r in cases),rejected=sum(not r['valid'] for r in cases),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest());(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(cases)} facility-definition controls passed ({receipt["accepted"]} accepted, {receipt["rejected"]} rejected)')
if __name__=='__main__':main()
