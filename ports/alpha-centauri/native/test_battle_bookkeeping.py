# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse,struct,subprocess,hashlib,json,os,random
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='g++');p.add_argument('--m68k-linux',action='store_true');a=p.parse_args();D=a.out.resolve();D.mkdir(parents=True,exist_ok=False);S=Path(__file__).resolve().parent
flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti']+(['-m68040','-static']if a.m68k_linux else['-fsanitize=address,undefined'])
subprocess.run([a.cc,*flags,str(S/'battle_bookkeeping.cpp'),str(S/'battle_bookkeeping_probe.cpp'),'-o',str(D/'probe')],check=True)
U=0xffffffff;seeds=[0,1,19,0x7ffffffe,0x7fffffff,0x80000000,0xfffffffe,U];rows=[]
def row(ed,at,de,outcome,mask,local,alien=False):
 k=mask%8;state=[(seeds[k]+j*13)&U for j in range(144)]
 for f in range(8):state.extend([0xa1a1a1a1,0xb2b2b2b2,0xc3c3c3c3,(mask>>f)&1,[0,1,2,U][(mask+f)%4],0xd4d4d4d4])
 win,lose=(de,at)if outcome==0 else(at,de)
 state[win]=seeds[k];state[8+lose]=seeds[(k+4)%8];state[16+win*8+lose]=seeds[k];state[80+win*8+lose]=seeds[(k+1)%8]
 ctx=[ed,at,de,outcome,local,0,0,1,seeds[k],[2100,0,1,0x80000000][(mask//4)%4]]+[128 if alien and f==win else 0 for f in range(8)]
 return ctx+state
for ed in range(2):
 for at in range(8):
  for de in range(8):
   if at==de:continue
   for outcome in range(2):
    for mask in [0,1,2,4,8,16,32,64,128,255,254,170,85,3,7,127]:
     win=de if outcome==0 else at
     for local in [-1,win]:rows.append(row(ed,at,de,outcome,mask,local,bool(mask&1)))
 # Every original monument prior-award mask, both battle outcomes and alien flags.
 for win in range(8):
  lose=(win+1)%8
  for mask in range(256):
   for outcome in range(2):
    at,de=(lose,win)if outcome==0 else(win,lose)
    rows.append(row(ed,at,de,outcome,mask,-1,bool(mask&1)))
rng=random.Random(20261027)
for _ in range(2000):
 ed=rng.randrange(2);at,de=rng.sample(range(8),2);r=row(ed,at,de,rng.randrange(2),rng.randrange(256),rng.randrange(-1,8),bool(rng.randrange(2)))
 r[18:]=[rng.getrandbits(32)for _ in range(192)];rows.append(r)
for field,values in [(0,[2,U]),(1,[8,U]),(2,[8,U]),(3,[2,U]),(4,[8,U-1]),(5,[1,2,U]),(6,[1,2,U]),(7,[0,2,U])]:
 for v in values:r=row(0,1,2,0,0,-1);r[field]=v;rows.append(r)
r=row(0,1,2,0,0,-1);r[2]=1;rows.append(r)
def signed(v):return v if v<0x80000000 else v-0x100000000
def expected(r):
 ed,at,de,kind,local,mp,pbem,obs,turn,start=r[:10];at,de,local=map(signed,[at,de,local]);s=r[18:]
 invalid=ed>1 or kind>1 or not 0<=at<=7 or not 0<=de<=7 or at==de or not -1<=local<=7 or mp>1 or pbem>1 or obs>1
 status=1 if invalid else 2 if obs==0 else 3 if mp or pbem else 0
 if status:return [status]+[0xa5a5a5a5]*387
 win,lose=(de,at)if kind==0 else(at,de);rec=lambda f:s[144+f*6:150+f*6]
 award=bool(win and lose and not rec(win)[3] and not(ed and r[10+win]&128))
 if award and local==win:return [4]+[0xa5a5a5a5]*387
 before=s.copy()
 for i in [win,8+lose]+([16+win*8+lose,80+win*8+lose]if kind else[]):before[i]=(before[i]+1)&U
 after=before.copy()
 if kind==0:
  for i in [16+win*8+lose,80+win*8+lose]:after[i]=(after[i]+1)&U
 if award:
  others=[f for f in range(8)if f!=win and rec(f)[3]]
  leader=lose if others else win
  for f in others:
   if rec(f)[4]:leader=f
  after[144+win*6:150+win*6]=[(turn+start)&U,lose,leader,1,0 if others else 1,6]
 return [0]+before+after+[win,lose,int(award)]
rows=[[v&U for v in r]for r in rows];wanted=[expected(r)for r in rows];pack=lambda seq:b''.join(struct.pack('<%dI'%len(r),*r)for r in seq)
data=pack(rows);want=pack(wanted);(D/'inputs.bin').write_bytes(data);(D/'expected.bin').write_bytes(want)
cmd=(['qemu-m68k','-cpu','m68040']if a.m68k_linux else[])+[str(D/'probe')];r=subprocess.run(cmd,input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});(D/'stderr').write_bytes(r.stderr);(D/'actual.bin').write_bytes(r.stdout);assert r.returncode==0,(r.returncode,r.stderr[:2000]);assert r.stdout==want,next((i,rows[i][:18],struct.unpack('<388I',r.stdout[i*1552:(i+1)*1552])[:1],wanted[i][:1])for i in range(len(rows))if r.stdout[i*1552:(i+1)*1552]!=want[i*1552:(i+1)*1552])
counts={str(s):sum(x[0]==s for x in wanted)for s in range(5)};receipt={'cases':len(rows),'statuses':counts,'overlapGuards':len(rows),'outputSHA256':hashlib.sha256(r.stdout).hexdigest(),'binarySHA256':hashlib.sha256((D/'probe').read_bytes()).hexdigest(),'native':'UNRUN'};(D/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
