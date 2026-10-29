# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent integer model of the original directed-human selection branch."""
def expected(row,visits=None):
 if visits is None:visits=[]
 fail=lambda e:[e]+[0x5a5a5a5a]*10
 edition,profile,pop,req,support,item,special,worked,unavailable=row[:9];sat=row[9:12];packed=row[12:14];minimum=row[14];citizens=[row[15+4*i:19+4*i] for i in range(7)];tiles=[row[43+3*i:46+3*i] for i in range(21)]
 if profile:return fail(4)
 if edition not in [0,1] or not 0<=pop<=127 or not 1<=req<=99 or not 0<=special<=pop or worked&~0x1fffff or unavailable&~0x1ffffff or worked.bit_count()+special>pop+1 or minimum<0 or any(not 0<=v<=pop for v in sat) or any(c[0] not in [0,1] for c in citizens):return fail(1)
 def check(n):
  if not -(1<<31)<=n<(1<<31):raise OverflowError
  return n
 def quotient(n,d):return abs(n)//d*(-1 if n<0 else 1)
 def clip(n):return min(99,max(1,n))
 try:
  remaining=pop+1;mask=0;total=sat[:];swapped=0
  def add(tile,sign=1):
   visits.append((1 if sign==1 else 2,tile))
   for k in range(3):total[k]=check(total[k]+sign*tiles[tile][k])
  for tile in range(21):
   if tile==0 or worked&(~unavailable|1)&(1<<tile):
    mask|=1<<tile;add(tile);remaining-=1
    if not remaining:break
  def value(tile):
   n,m,e=tiles[tile];demand=check(pop*req);extra=0
   if check(remaining//req+total[0])<demand:
    extra=2 if check(total[0]+remaining)>=demand else 8
    if n>=0:n=check(n*2 if n<=extra else n+extra)
   n=check(n*2);m=check(m*2);e=check(e*2)
   if total[0]<=check(pop+demand) and n==0:m=quotient(m,2);e=quotient(e,2)
   if m==0 and not extra:n=quotient(n,2)
   dn=check(total[0]-pop//3);dn=check(dn-demand);dn=check(dn+remaining);dn=check(dn*2);dn=clip(check(dn-2))
   dm=clip(quotient(check(check(total[1]-support)+1),2));de=clip(total[2])
   food=quotient(check(check(n*4)*256),dn)
   coefficient=8 if item<-(70 if edition else 39) else 6
   mineral=quotient(check(check(check(coefficient*pop)*m)*64),dm)
   energy=quotient(check(check(pop*e)*64),de)
   return check(check(food+mineral)+energy)
  if remaining!=special:
   while True:
    complete=remaining<=special;choices=[];current=[]
    for tile in range(1,21):
     if unavailable&(1<<tile) or (not complete and mask&(1<<tile)):continue
     visits.append((0,tile));s=value(tile)
     if mask&(1<<tile):current.append((s,tile))
     else:choices.append((s,tile))
    best=max(choices,key=lambda v:(v[0],-v[1]),default=(0,0));best=best if best[0]>0 else (0,0)
    worst=min(current,key=lambda v:(v[0],v[1]),default=(9999,-1));worst=worst if worst[0]<9999 else (9999,-1)
    if complete:
     if best[1]>0 and worst[1]>0 and worst[0]<best[0]:
      previous=total[:];add(worst[1],-1);add(best[1])
      if total[0]<check((pop+1)*req):
       visits.extend([(2,best[1]),(1,worst[1])]);total[:]=previous
      else:mask=(mask&~(1<<worst[1]))|(1<<best[1]);swapped=1
     break
    if not best[1]:break
    mask|=1<<best[1];add(best[1]);remaining-=1
    if not(special<remaining or total[0]<check(pop*req)):break
  if remaining>special:
   ranked=[]
   for i,(available,econ,psych,labs) in enumerate(citizens):
    if available:
     v=check(psych*3)
     if pop>=minimum:v=check(check(v+econ)+labs)
     if v> -999:ranked.append((v,i))
   best=max(ranked,key=lambda v:(v[0],-v[1]),default=(-999,0))[1]
   for at in range(special,min(16,remaining)):
    index,slot=divmod(at,8);packed[index]=(packed[index]&~(15<<(slot*4)))|(best<<(slot*4))
  return [0,mask,remaining,0,*total,*packed,swapped,int(total[0]<check(pop*req))]
 except OverflowError:return fail(2)
