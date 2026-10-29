#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent explicit-byte oracle; synthetic inputs public, owner inputs optional."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
SRC=Path(__file__).resolve().parent

def tables():
 tech=[[f'Tech{i}',f'T{i}','0','1','2','3','None','None','1'] for i in range(89)]
 rows=[[f'Citizen{i}',f'Citizens{i}',('None' if i%3==0 else f'T{i}'),('Disable' if i%2==0 else f'T{i+1}'),str(i-3),str(2-i),str(i), 'ignored flags'] for i in range(7)]
 rows += [[f'Citizen{i}',f'Citizens{i}'] for i in range(7,10)]
 return tech,rows

def material(tech,rows,tail='#NEXT'):
 return ('#TECHNOLOGY\n'+'\n'.join(','.join(r) for r in tech)+'\n#CITIZENS\n'+'\n'.join(','.join(r) for r in rows)+'\n'+tail+'\n').encode('latin1')

def reference(data,variant):
 tables={};name=None
 for line in data.decode('latin1').splitlines():
  line=line.strip()
  if not line or line.startswith(';'):continue
  if line.startswith('#'):name=line[1:].lower();tables[name]=[];continue
  if name:tables[name].append([x.strip() for x in (line.split(';')[0] if name=='technology' else line).split(',')])
 keys={r[1].lower():i for i,r in enumerate(tables['technology'])};keys.update(none=-1,disable=-2)
 rows=tables['citizens'];assert len(rows)==10
 result=bytearray(struct.pack('<II',10,variant=='expansion'))
 def number(s):
  if s.startswith('+'):s=s[1:]
  return int(s[2:],{'b':2,'x':16,'d':10}[s[1].lower()]) if len(s)>2 and s[0]=='0' and s[1].lower() in 'bxd' else int(s)
 for i,row in enumerate(rows):
  result+=row[0].encode('latin1').ljust(128,b'\0')+row[1].encode('latin1').ljust(128,b'\0')
  result+=struct.pack('<5i',*([keys[row[2].lower()],keys[row[3].lower()],*[number(x) for x in row[4:7]]] if i<7 else [-2,-2,0,0,0]))
 return bytes(result)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True,type=Path);ap.add_argument('--cc',default='c++');ap.add_argument('--m68k-linux',action='store_true');ap.add_argument('--original',action='append',default=[]);a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
 exe=out/'citizens-probe';cmd=[a.cc,*flags,str(SRC/'rules_text.cpp'),str(SRC/'citizens_text.cpp'),str(SRC/'citizens_probe.cpp'),'-o',str(exe)];subprocess.run(cmd,check=True);subprocess.run([*runner,str(exe),'--selftest'],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));cases=[];digest=hashlib.sha256()
 def check(name,data,variant,valid=True):
  path=out/(str(len(cases))+'.txt');path.write_bytes(data)
  run=subprocess.run([*runner,str(exe),variant,str(path)],capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),timeout=30)
  if valid:
   wanted=reference(data,variant);assert run.returncode==0 and run.stdout==wanted,(name,run.returncode,run.stderr,len(run.stdout),len(wanted));digest.update(run.stdout)
  else:assert run.returncode==20 and not run.stdout and b'ERROR' in run.stderr,(name,run.returncode,run.stderr)
  cases.append(dict(name=name,variant=variant,accepted=valid,sha256=hashlib.sha256(run.stdout).hexdigest()))
 for variant in ('base','expansion'):
  tech,rows=tables();data=material(tech,rows);check('standard',data,variant)
  for sep in (b'\r',b'\r\n'):check('line-ending',data.replace(b'\n',sep),variant)
  check('eof',material(tech,rows,tail=''),variant)
  check('comments',data.replace(b'#CITIZENS',b'#citizens\n; comment\n\n'),variant)
  check('missing-section',data.replace(b'#CITIZENS',b'#OTHER'),variant,False)
  for n in range(10):check('missing-row-'+str(n),material(tech,rows[:n]),variant,False)
  check('extra-row',material(tech,rows+[rows[0]]),variant,False)
  check('nul',data.replace(b'Citizen0',b'Citiz\0n0'),variant,False)
  check('long-line',data.replace(b'Citizen0',b'x'*512),variant,False)
  for i in range(10):
   for col in (0,1):
    for val,valid in [('',True),('x'*127,True),('x'*128,False),('one;two',True)]:
     edit=[r.copy() for r in rows];edit[i][col]=val;check(f'name-{i}-{col}',material(tech,edit),variant,valid)
  for i in range(7):
   for col in (2,3):
    for val,valid in [('nOnE',True),('dIsAbLe',True),('t88',True),('BAD',False),('',False),('toolongkey',False)]:
     edit=[r.copy() for r in rows];edit[i][col]=val;check(f'tech-{i}-{col}',material(tech,edit),variant,valid)
   for col in (4,5,6):
    for val,valid in [('-2147483648',True),('2147483647',True),('+17',True),('0x7fffffff',True),('0b101',True),('0d18',True),('2147483648',False),('-2147483649',False),('1x',False),('',False),('-',False),('0x',False),('1;tail',False)]:
     edit=[r.copy() for r in rows];edit[i][col]=val;check(f'number-{i}-{col}',material(tech,edit),variant,valid)
   edit=[r.copy() for r in rows];edit[i]=edit[i][:7]+['nonsense;anything','extra'];check('unused-specialist-tail',material(tech,edit),variant)
   for n in range(2,7):
    edit=[r.copy() for r in rows];edit[i]=edit[i][:n];check('missing-field',material(tech,edit),variant,False)
  for i in range(7,10):
   edit=[r.copy() for r in rows];edit[i]+=['garbage','99999999999999999999'];check('unused-name-row-tail',material(tech,edit),variant)
 public_digest=digest.hexdigest();public_count=len(cases)
 for spec in a.original:
  variant,file=spec.split(':',1);p=Path(file);before=p.read_bytes();check('owner-'+p.name,before,variant);assert p.read_bytes()==before
 receipt=dict(publicCount=public_count,publicOutputSHA256=public_digest,directControls=7,cases=cases,accepted=sum(c['accepted'] for c in cases),rejected=sum(not c['accepted'] for c in cases),outputSHA256=digest.hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=cmd)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(f'{len(cases)} citizen controls: {receipt["accepted"]} accepted, {receipt["rejected"]} rejected; {digest.hexdigest()}')
if __name__=='__main__':main()
