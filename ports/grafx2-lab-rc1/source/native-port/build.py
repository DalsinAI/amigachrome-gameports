from pathlib import Path
import subprocess,os,concurrent.futures,hashlib,json
base=Path(__file__).resolve().parent
work=Path(os.environ.get('GRAFX2_WORK',base/'.work'))
tc=Path(os.environ['OPENAMIGA_GCC'])/'bin'
cc=str(tc/'m68k-amigaos-gcc');ar=str(tc/'m68k-amigaos-ar')
identity=subprocess.check_output([cc,'--version'],text=True)
if 'OpenAmigaGCC 16.2.0-open3' not in identity: raise SystemExit('OpenAmigaGCC open3 is required')
ndk=Path(os.environ['AMIGA_NDK']);src=work/'grafx2/src';out=work/'build';out.mkdir(parents=True,exist_ok=True)
sdk_digest=hashlib.sha256(b''.join(h.read_bytes() for h in sorted((base/'sdk').rglob('*.h')))).hexdigest()
ndk_digest=hashlib.sha256(b''.join(h.read_bytes() for h in sorted(ndk.rglob('*.h')))).hexdigest()
flags=['-m68040','-m68881','-mcrt=nix20','-D__USE_NEW_TIMEVAL__','-O2','-fomit-frame-pointer','-fno-late-combine-instructions','-fno-delete-null-pointer-checks','-I'+str(ndk),'-I'+str(base/'sdk')]
def run(cmd):
 r=subprocess.run(cmd,capture_output=True,text=True)
 if r.returncode: print('FAILED:', ' '.join(cmd), '\n',r.stderr[:8000],flush=True);raise RuntimeError(cmd[-1])
 if r.stderr: print(r.stderr,flush=True)
 return r

def compilefile(p,extra=[]):
 o=out/(p.stem+'.o')
 fingerprint = hashlib.sha256((identity+ndk_digest+sdk_digest+str(flags+extra)+p.read_text()+''.join(h.read_text() for h in sorted(src.glob('*.h')))+''.join(h.read_text() for h in sorted(src.glob('*.inc')))).encode()).hexdigest()
 stamp = o.with_suffix('.sha256')
 if not o.exists() or not stamp.exists() or stamp.read_text()!=fingerprint:
  run([cc,*flags,*extra,'-c',str(p),'-o',str(o)])
  stamp.write_text(fingerprint)
 return str(o)
text=(src/'Makefile').read_text();objects=text.split('OBJS = ',1)[1].split('ifndef NORECOIL',1)[0].replace('\\\n',' ')
objects=objects.replace('$(APIOBJ)','amigascreen.o amigadatatype.o amigafpu.o amigaopenup.o amigafont.o amigaexpand.o zlibrary.o').replace('$(PLATFORMOBJ)','')
files=[src/(f[:-2]+'.c') for f in objects.split()]
# bundled upstream 6502 emulator
extra=['-DUSE_AMIGA_NATIVE','-DUSE_OPENUP','-DNORECOIL','-DNOTTF=1','-D__no_tifflib__','-D__no_pnglib__']
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 objs=list(pool.map(lambda p:compilefile(p,extra + (['-DCPU_6502_STATIC','-DCPU_6502_USE_LOCAL_HEADER','-DCPU_6502_DEPENDENCIES_H="6502types.h"'] if p.name == '6502.c' else [])),files))
run([cc,*flags,*objs,'-lm','-lamiga','-Wl,-Map,'+str(out/'GrafX2.map'),'-o',str(out/'GrafX2')])
print('LINKED',out/'GrafX2')
(out/'build-info.json').write_text(json.dumps({
 'compiler':identity.strip(), 'flags':flags+extra,
 'upstream':'678a0f54ea7f3eeebc3fa4135213ea63f45a28bd',
 'patches':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((base/'patches').glob('*.patch'))},
 'ndk_headers_sha256':ndk_digest,'sdk_headers_sha256':sdk_digest,
 'binary_sha256':hashlib.sha256((out/'GrafX2').read_bytes()).hexdigest()
},indent=2)+'\n')
