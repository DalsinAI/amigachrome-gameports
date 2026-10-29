#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from human_allocation_reference import expected
from test_human_worker_allocation import cases
HERE=Path(__file__).resolve().parent

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=False);exe=a.out/'human-allocation-trace';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror'];runner=[]
 if a.m68k_linux:flags+=['-m68040','-static'];runner=['qemu-m68k','-cpu','m68040']
 else:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 command=[a.cc,*flags,str(HERE/'human_worker_allocation.cpp'),str(HERE/'human_allocation_trace_probe.cpp'),'-o',str(exe)];subprocess.run(command,check=True);rows=cases();answers=[];traces=[]
 for row in rows:
  visits=[];answers.append(expected(row,visits));traces.append(visits if answers[-1][0]==0 else [])
 data=''.join(' '.join(map(str,r))+'\n' for r in rows).encode();(a.out/'input.txt').write_bytes(data);wanted=b''.join(struct.pack('<11I',*[v&0xffffffff for v in row])+struct.pack('<I',len(trace))+b''.join(struct.pack('<II',*event) for event in trace) for row,trace in zip(answers,traces));r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=120,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(a.out/'actual.bin').write_bytes(r.stdout);(a.out/'expected.bin').write_bytes(wanted);(a.out/'stderr.txt').write_bytes(r.stderr)
 assert r.returncode==0 and r.stdout==wanted,(r.returncode,r.stderr,next((i for i,(x,y) in enumerate(zip(r.stdout,wanted)) if x!=y),None))
 report=dict(visits=sum(map(len,traces)),maxVisits=max(map(len,traces)),rollbacks=sum(any(trace[i:i+4]==[(2,trace[i][1]),(1,trace[i+1][1]),(2,trace[i+1][1]),(1,trace[i][1])] for i in range(len(trace)-3)) for trace in traces),cases=len(rows),accepted=sum(x[0]==0 for x in answers),rejected=sum(x[0]!=0 for x in answers),exchanges=sum(x[0]==0 and x[9] for x in answers),foodDeficits=sum(x[0]==0 and x[10] for x in answers),outputSHA256=hashlib.sha256(wanted).hexdigest(),binarySHA256=hashlib.sha256(exe.read_bytes()).hexdigest(),command=command,native='UNRUN');(a.out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='command'}))
if __name__=='__main__':main()
