#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Existing coherent specialist arrangement through owned accounting and full bytes.

Uses the independent current-wire resource/commerce/citizen/psych models in the
shared driver; it does not implement the production yield/count algorithm.
"""
import struct,sys
import test_local_accounting as transaction
large="--large" in sys.argv
if large:sys.argv.remove("--large")
original_fixture=transaction.fixture
def fixture(version,case):
 b=bytearray(original_fixture(version,case));info,o,_=transaction.e.population.decode(b);base=o['bases'];pop=b[base+6]
 if large:pop=[16,17,31,63,127][(case-1)%5];b[base+6]=pop
 workers=1+(case//5)%3 if large else 1 if pop==0 else 1+(case//6)%pop
 specialists=pop+1-workers
 struct.pack_into('<IiiII',b,base+0x78,(1<<workers)-1,specialists,0,0x01234560,0x65432106)
 # Dedicated malformed count/adjustment/packed-specialist controls.
 if case==24:struct.pack_into('<i',b,base+0x7c,pop+1)
 if case==25:struct.pack_into('<i',b,base+0x80,1)
 if case==26:struct.pack_into('<I',b,base+0x84,0xffffffff)
 return bytes(b)
transaction.fixture=fixture
if __name__=='__main__':transaction.main()
