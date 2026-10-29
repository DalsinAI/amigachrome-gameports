# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,os,itertools
HERE=Path(__file__).resolve().parent
SOURCES=['fungus_combat.cpp','fungus_combat_probe.cpp']
def cases():
 for off,uid,weapon,defoff,alt,items,faction,pholus in itertools.product([864,-3,1,1400000000,2147483647,-2147483648],[15,63,64,320],[-1,3],[-1,0,3],[32,63,64,96],[0,32],[0,1],[-1,0,1,2]):yield[off,uid,weapon,defoff,alt,items,faction,pholus,0]
 b=[864,15,-1,3,96,32,1,-1,0]
 for idx,values in[(1,[-1,512]),(2,[-129,128]),(3,[-129,128]),(4,[-1,256]),(6,[-1,8]),(7,[-2,8]),(8,[-1,11,256])]:
  for value in values:c=b.copy();c[idx]=value;yield c
def model(c):
 off,uid,weapon,defoff,alt,items,faction,pholus,plan=c
 if not(0<=uid<512 and -128<=weapon<=127 and -128<=defoff<=127 and 0<=alt<=255 and 0<=faction<=7 and -1<=pholus<=7 and 0<=plan<=255 and plan!=11):return[1,0,0,3]
 use=bool(items&32 and alt>=64 and(uid<64 and weapon<0 or pholus==faction)and defoff>=0)
 result=off+(abs(off)//2*(-1 if off<0 else 1))if use else off
 if not -2**31<=result<2**31:return[2,0,0,3]
 return[0,result,int(use),2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);exe=out/'probe';flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']);cmd=[a.cc,*flags,*[str(HERE/x)for x in SOURCES],'-o',str(exe)];subprocess.run(cmd,check=True);cs=list(cases());ms=[model(c)for c in cs];data=b''.join(struct.pack('<9I',*(x&0xffffffff for x in c))for c in cs);want=b''.join(struct.pack('<4I',*(x&0xffffffff for x in m))for m in ms);(out/'inputs.bin').write_bytes(data);(out/'expected.bin').write_bytes(want);runner=['qemu-m68k','-cpu','m68040']if a.m68k_linux else[];r=subprocess.run([*runner,str(exe)],input=data,capture_output=True,timeout=180,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'output.bin').write_bytes(r.stdout);(out/'stderr').write_bytes(r.stderr);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,next((i for i,(x,y)in enumerate(zip(r.stdout,want))if x!=y),None));d={'cases':len(cs),'accepted':sum(m[0]==0 for m in ms),'invalid':sum(m[0]==1 for m in ms),'overflow':sum(m[0]==2 for m in ms),'guards':sum(m[3]for m in ms),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256(exe.read_bytes()).hexdigest(),'command':cmd,'native':'UNRUN'};(out/'receipt.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
if __name__=='__main__':main()
