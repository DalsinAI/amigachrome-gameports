# SPDX-License-Identifier: GPL-3.0-or-later
"""Procedural CVR lighting controls, without original executable/table data."""
import argparse,hashlib,json,math,os,random,struct,subprocess
from pathlib import Path

def lattice():
 out=bytearray()
 for t in range(24):
  for j in range(2137):
   polar=min(23,(j+88)//89)*math.pi/46;az=((j+88)%89)*2*math.pi/89;light=t*math.pi/46
   dot=abs(math.cos(polar)*math.cos(light)+math.sin(polar)*math.sin(light)*math.cos(az))
   out.append(max(0,min(23,int(.5+23-math.acos(min(1,dot))*46/math.pi))))
 return bytes(out)
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def directions():
 out=[(x,y,z) for x in (0.,-0.) for y in (0.,-0.) for z in (1.,-1.)]
 out += [(x,y,z) for x in (1.,-1.) for y in (0.,-0.) for z in (0.,-0.)]
 out += [(x,y,z) for x in (0.,-0.) for y in (1.,-1.) for z in (0.,-0.)]
 out += [(0.,0.,0.),(0.,0.,2.),(100.,0.,.5),(.25,-.5,.25)]
 for theta in (0,.13,.42,.8,1.2,math.pi/2):
  for phi in (0,.2,1,2,3,4,5,6.1):
   for sign in (1,-1):out.append(tuple(f32(v) for v in (math.cos(phi)*math.sin(theta),math.sin(phi)*math.sin(theta),sign*math.cos(theta))))
 r=random.Random(0x040090)
 for _ in range(256):
  z=r.uniform(-1,1);phi=r.uniform(0,2*math.pi);xy=math.sqrt(1-z*z)
  out.append(tuple(f32(v) for v in (xy*math.cos(phi),xy*math.sin(phi),z)))
 return out

def expected(table,vecs):
 out=bytearray(table)
 for x,y,z in vecs:
  if z<0:x,y,z=-x,-y,-z
  az=math.atan2(y,x)
  if az<0:az-=f32(-2*math.pi)
  t=0 if z>=1 else int(.5+math.acos(z)*f32(46/math.pi))
  p=int(.5+az*f32(89/f32(2*math.pi)));p=0 if p==89 else p
  assert 0<=t<24 and 0<=p<89
  start=t*2137+89-p
  out+=struct.pack('<II',t,p)+table[start:start+2048]
 return bytes(out)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 table=lattice();assert hashlib.sha256(table).hexdigest()=='67e38d0ee2d10fc892b5e9bf82af902510ad53bf2c2d9e531792cf2634684054'
 vecs=directions();(out/'input.bin').write_bytes(b'CVRL'+struct.pack('<I',len(vecs))+b''.join(struct.pack('<3f',*v) for v in vecs));want=expected(table,vecs)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined']
 prefix=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 for name in ['probe','control']:
  subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'cvr_lighting.cpp'),str(src/f'cvr_lighting_{name}.cpp'),'-lm','-o',str(out/name)],check=True)
 p=subprocess.run(prefix+[str(out/'probe'),str(out/'input.bin')],capture_output=True,env=env);assert p.returncode==0,(p.returncode,p.stderr)
 assert p.stdout==want,(len(p.stdout),len(want),next((i for i,(v,w) in enumerate(zip(p.stdout,want)) if v!=w),None))
 c=subprocess.run(prefix+[str(out/'control')],capture_output=True,env=env);assert c.returncode==0 and c.stdout==b'CVR LIGHTING CONTROL cases=15 RC 0\n',(c.returncode,c.stdout,c.stderr)
 (out/'output.bin').write_bytes(p.stdout);receipt=dict(latticeEntries=len(table),directions=len(vecs),invalidControls=15,latticeSHA256=hashlib.sha256(table).hexdigest(),outputSHA256=hashlib.sha256(want).hexdigest());(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
