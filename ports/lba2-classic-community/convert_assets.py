#!/usr/bin/env python3
"""Build versioned big-endian LBA2 animation/body caches from imported HQR banks.

Copyright (c) 2026 Dalsin Limited. GPL-2.0-only.
Schemas follow the pinned engine's ANIM_LE.CPP and BODY_LE.CPP. No retail assets
are shipped. Originals are never changed. Unsupported or malformed entries fail.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile
import zlib

MAGIC=b'LBA2BE01'
BANKS=('ANIM.HQR','BODY.HQR','OBJFIX.HQR')
MAX_ENTRY=16*1024*1024


def decode_entry(bank, index):
    if len(bank)<4:
        raise ValueError('short HQR')
    table=struct.unpack_from('<I',bank)[0]
    if table%4 or not 4<=table<=len(bank) or not 0<=index<table//4-1:
        raise ValueError('invalid HQR table/index')
    off=struct.unpack_from('<I',bank,index*4)[0]
    if off==0:
        return b''
    if not table<=off<=len(bank)-10:
        raise ValueError('entry offset outside HQR')
    size,packed,method=struct.unpack_from('<IIH',bank,off)
    if size>MAX_ENTRY or off+10+packed>len(bank) or method>2:
        raise ValueError('unsupported or invalid HQR entry')
    src=bank[off+10:off+10+packed]
    if method==0:
        if size!=packed:
            raise ValueError('stored size mismatch')
        return src
    dst=bytearray();p=0
    while len(dst)<size:
        if p>=len(src):
            raise ValueError('truncated LZ flags')
        flags=src[p];p+=1
        for bit in range(8):
            if len(dst)==size:
                break
            if flags&(1<<bit):
                if p>=len(src):
                    raise ValueError('truncated LZ literal')
                dst.append(src[p]);p+=1
            else:
                if p+2>len(src):
                    raise ValueError('truncated LZ reference')
                length=(src[p]&15)+method+1
                distance=((src[p+1]<<4)|(src[p]>>4))+1;p+=2
                if distance>len(dst):
                    raise ValueError('LZ reference before output')
                for _ in range(min(length,size-len(dst))):
                    dst.append(dst[-distance])
    return bytes(dst)


def native_spans(data, kind):
    """Byte ranges of scalar values; texture pixels and padding stay untouched."""
    if kind=='anim':
        if len(data)<8 or len(data)%2:
            raise ValueError('invalid animation length')
        frames,groups,loop,_=struct.unpack_from('<4H',data)
        if 8+frames*(8+groups*8)>len(data) or (frames and loop>=frames):
            raise ValueError('animation frames outside resource')
        return [(0,len(data),2)]
    if len(data)<96:
        raise ValueError('short body')
    header=struct.unpack_from('<i22i',data[:4]+data[8:96])
    spans=[(0,4,4),(4,4,2),(8,88,4)]
    # Six bounds, then count/offset pairs in T_BODY_HEADER.
    sections=[(header[7+2*i],header[8+2*i]) for i in range(8)]
    for i,(count,off) in enumerate(sections):
        if i==4: # polygons have per-block schemas
            continue
        width=4 if i==7 else 2
        record=4 if i==7 else 8
        if count==0:
            continue
        if count<0 or off<96 or off+count*record>len(data):
            raise ValueError('body section outside resource')
        spans.append((off,count*record,width))
    off,end=sections[4][1],sections[5][1]
    if off<96 or end<off or end>len(data):
        raise ValueError('body polygon extent invalid')
    while off+8<=end:
        typ,count,_=struct.unpack_from('<HHI',data,off)
        rec=16 if typ&0x4000 else 12 if typ&255<=7 else 32 if typ&0x8000 else 24
        size=count*rec
        if off+8+size>end:
            raise ValueError('body polygon records outside section')
        spans.extend([(off,4,2),(off+4,4,4),(off+8,size,2)])
        off+=8+size
    # Reject overlapping schemas, including damaged section offsets.
    last=0
    for start,size,width in sorted(spans):
        if size and start<last:
            raise ValueError('overlapping native fields')
        last=max(last,start+size)
    return spans


def convert(data,kind):
    if not data:
        return data
    out=bytearray(data)
    spans=native_spans(data,kind)
    for start,size,width in spans:
        for pos in range(start,start+size,width):
            out[pos:pos+width]=data[pos:pos+width][::-1]
    # Exact reversal also checks that byte-preserved regions survive.
    back=bytearray(out)
    for start,size,width in spans:
        for pos in range(start,start+size,width):
            back[pos:pos+width]=out[pos:pos+width][::-1]
    if back!=data:
        raise ValueError('native conversion roundtrip failed')
    return bytes(out)


def build_bank(bank,name):
    if len(bank)<4:
        raise ValueError('short bank')
    count=struct.unpack_from('<I',bank)[0]//4-1
    if not 0<count<100000:
        raise ValueError('invalid resource count')
    payload=bytearray();entries=[]
    base=24+count*12
    for i in range(count):
        try:
            data=convert(decode_entry(bank,i),'anim' if name=='ANIM.HQR' else 'body')
        except ValueError as e:
            raise ValueError(f'{name} entry {i}: {e}') from e
        entries.append((base+len(payload),len(data),zlib.crc32(data)))
        payload.extend(data)
    header=MAGIC+struct.pack('>4I',len(bank),zlib.crc32(bank),count,0)
    return header+b''.join(struct.pack('>3I',*e) for e in entries)+payload,count


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--data',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if out.exists():
        raise ValueError('use a fresh output directory')
    out.parent.mkdir(parents=True,exist_ok=True)
    tmp=Path(tempfile.mkdtemp(prefix=out.name+'.',dir=out.parent))
    receipt={'schema':'lba2-native-cache','version':1,'byte_order':'big','banks':{},'qualification':'not runtime-qualified',
             'converter_source_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
             'upstream_commit':'2fa1c7a66de2ce8707d85cb9a304dbd6f96c8ab8'}
    for name in BANKS:
        matches=[f for f in a.data.rglob('*') if f.is_file() and f.name.upper()==name]
        if len(matches)!=1:
            raise ValueError(f'expected exactly one {name}, found {len(matches)}')
        bank=matches[0].read_bytes();cache,count=build_bank(bank,name)
        (tmp/(name+'.bec')).write_bytes(cache)
        receipt['banks'][name]={'source_sha256':hashlib.sha256(bank).hexdigest(),'source_bytes':len(bank),
            'cache_sha256':hashlib.sha256(cache).hexdigest(),'cache_bytes':len(cache),'entries':count}
    (tmp/'cache.json').write_text(json.dumps(receipt,indent=2)+'\n')
    os.rename(tmp,out)
    print(json.dumps(receipt,indent=2))

if __name__=='__main__':
    main()
