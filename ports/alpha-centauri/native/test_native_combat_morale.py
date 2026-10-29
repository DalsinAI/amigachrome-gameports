# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['native_combat_morale.cpp','native_combat_morale_probe.cpp']
def model(c):
 v,turn,unit,opponent,atrocity,tectonic,state,flags,count=c
 bad=v not in [0,1] or not 0<=unit<512 or not -1<=opponent<=7 or not 0<=flags<=65535 or not 0<=count<=8
 if bad:return [1,0x12345678,3]
 m=next((i for i,t in enumerate([45,90,170,250,330]) if turn<t),6)
 m+=int(opponent>0 and atrocity!=0)+int(v==1 and opponent>0 and tectonic!=0)+int(bool(state&256))+int(unit==10)+((flags>>3)&3)
 if v==1 and unit==19:m=count-2
 return [0,max(0,min(6,m)),2]
def cases():
 for i,c in enumerate(itertools.product(range(2),[-2**31,44,45,89,90,169,170,249,250,329,330,2**31-1],[0,10,19,511],[-1,0,1,7],[-1,0,1],[0,-2**31],[0,256],[0,8,16,65535])):yield [*c,(i//7)%9]
 base=[0,0,0,1,0,0,0,0,0]
 for field,values in [(0,[-1,2]),(2,[-1,512]),(3,[-2,8]),(7,[-1,65536]),(8,[-1,9])]:
  for value in values:c=base.copy();c[field]=value;yield c

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static'] if a.m68k_linux else ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x) for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());data=b''.join(struct.pack('<9i',*c) for c in cs);expected=b''.join(struct.pack('<3I',*model(c)) for c in cs);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(expected);runner=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==expected,(r.returncode,r.stderr,next((i for i,(x,y) in enumerate(zip(r.stdout,expected)) if x!=y),None));ms=[model(c) for c in cs];receipt={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'rejected':sum(m[0]!=0 for m in ms),'guards':sum(m[2] for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
