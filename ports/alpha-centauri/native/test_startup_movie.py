# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent container/pixel tests. No original movie data is included."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import prepare_startup_movie as prepare

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',required=True);ap.add_argument('--cc',default='g++');ap.add_argument('--m68k-linux',action='store_true');a=ap.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=False);src=Path(__file__).resolve().parent
 w=h=128;y=bytearray(w*h);u=bytearray(w*h//4);v=bytearray(w*h//4)
 for b in range(4096):
  xx=(b%64)*2;yy=(b//64)*2;u[b]=(b//16%16)*17;v[b]=(b//256)*17
  for dy in range(2):
   for dx in range(2):y[(yy+dy)*w+xx+dx]=(b%16)*17
 want=bytearray()
 for yy in range(h):
  for xx in range(w):
   lum=y[yy*w+xx]-16;cb=u[(yy//2)*64+xx//2]-128;cr=v[(yy//2)*64+xx//2]-128
   want.extend(max(0,min(255,q//256)) for q in (298*lum+409*cr+128,298*lum-100*cb-208*cr+128,298*lum+516*cb+128))
 data=b'AMOVYUV1'+struct.pack('<8I',w,h,1,22050,2,1470,1470,0)+y+u+v+bytes(1470*4);(out/'input.amov').write_bytes(data)
 flags=['-m68040','-static'] if a.m68k_linux else ['-g','-fsanitize=address,undefined'];prefix=['qemu-m68k','-cpu','m68040'] if a.m68k_linux else [];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 for name in ('control','probe'):
  subprocess.run([a.cc,'-std=c++17','-O1','-Wall','-Wextra','-Werror',*flags,str(src/'startup_movie.cpp'),str(src/f'startup_movie_{name}.cpp'),'-o',str(out/name)],check=True)
 c=subprocess.run(prefix+[str(out/'control')],capture_output=True,env=env);assert c.returncode==0,(c.returncode,c.stdout,c.stderr)
 p=subprocess.run(prefix+[str(out/'probe'),str(out/'input.amov')],capture_output=True,env=env);assert p.returncode==0 and p.stdout==want,(p.returncode,len(p.stdout),p.stderr)
 # Partial final EA block: normalize only its sample-count field. Payload stays exact.
 chunk=lambda tag,body:tag+struct.pack('<I',len(body)+8)+body
 payload=struct.pack('<I',14)+bytes(8)+bytes(range(30));movie=chunk(b'SCHl',b'')+chunk(b'SCDl',payload)+chunk(b'pIQT',b'')+chunk(b'SCEl',b'');(out/'tail.wve').write_bytes(movie)
 frames,samples,fixes=prepare.normalize_ea_tail(out/'tail.wve',out/'tail-normalized.wve');expected=movie[:16]+struct.pack('<I',28)+movie[20:];assert (frames,samples,fixes)==(1,14,[dict(declared=14,padded=28)]) and (out/'tail-normalized.wve').read_bytes()==expected
 for suffix in (b'X',chunk(b'SCDl',payload)):
  (out/'bad.wve').write_bytes(movie+suffix)
  try:prepare.normalize_ea_tail(out/'bad.wve',out/'bad-normalized.wve')
  except ValueError:pass
  else:raise AssertionError('Malformed data accepted')
 receipt=dict(control=c.stdout.decode().strip(),independent_pixel_values=w*h*3,pixel_sha256=hashlib.sha256(want).hexdigest(),tail_controls=3);(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
