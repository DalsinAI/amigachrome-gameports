"""Adapt pinned wasm2c 1.0.41 output to forward little-endian memory.

The native host sees the same byte order as WASI files and the wasm module.
No SIMD, shared-memory or multi-instance/thread support is claimed.
"""
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
helper = '''
/* Keep module bytes little-endian and forward-addressed on every host. */
static inline void civ2_wasm_scalar_copy(void* to, const void* from, size_t n) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  unsigned char* d = (unsigned char*)to;
  const unsigned char* s = (const unsigned char*)from;
  for (size_t i = 0; i < n; ++i) d[i] = s[n - i - 1];
#else
  wasm_rt_memcpy(to, from, n);
#endif
}
'''
count = 0
for source in list(root.glob('*.c')) + list(root.glob('*.h')):
    data = source.read_text()
    if '#define DEFINE_LOAD(name, t1, t2, t3, force_read)' not in data:
        continue
    if 'civ2_wasm_scalar_copy' in data:
        raise SystemExit(f'Already adapted: {source}')
    data = data.replace('#define DEFINE_LOAD(name, t1, t2, t3, force_read)',
                        helper + '\n#define DEFINE_LOAD(name, t1, t2, t3, force_read)', 1)
    for old, new in [
        ('wasm_rt_memcpy(&result, MEM_ADDR_MEMOP(mem, addr, sizeof(t1)),',
         'civ2_wasm_scalar_copy(&result, MEM_ADDR_MEMOP(mem, addr, sizeof(t1)),'),
        ('wasm_rt_memcpy(MEM_ADDR_MEMOP(mem, addr, sizeof(t1)), &wrapped,',
         'civ2_wasm_scalar_copy(MEM_ADDR_MEMOP(mem, addr, sizeof(t1)), &wrapped,'),
    ]:
        if data.count(old) != 1:
            raise SystemExit(f'Unexpected wasm2c template: {source}: {old}')
        data = data.replace(old, new, 1)
    source.write_text(data)
    count += 1
if count != 1:
    raise SystemExit(f'Expected one memory implementation, found {count}')

runtime = root / 'wasm-rt.h'
data = runtime.read_text()
old = '#ifdef WASM_RT_C11_AVAILABLE\n#define WASM_RT_THREAD_LOCAL _Thread_local'
new = '''#if defined(CIV2_WASM_SINGLE_THREAD)
/* Exactly one module on one guest task; no threaded callers are supported. */
#define WASM_RT_THREAD_LOCAL
#elif defined(WASM_RT_C11_AVAILABLE)
#define WASM_RT_THREAD_LOCAL _Thread_local'''
if data.count(old) != 1:
    raise SystemExit('Unexpected wasm-rt TLS definition')
data = data.replace(old, new, 1)
data = data.replace('#ifndef _WIN32\n#define WASM_RT_SETJMP_TRAP_SETBUF',
                    '#if !defined(_WIN32) && !defined(CIV2_WASM_SINGLE_THREAD)\n#define WASM_RT_SETJMP_TRAP_SETBUF', 1)
data = data.replace('#ifndef _WIN32\n#define WASM_RT_LONGJMP_UNCHECKED',
                    '#if !defined(_WIN32) && !defined(CIV2_WASM_SINGLE_THREAD)\n#define WASM_RT_LONGJMP_UNCHECKED', 1)
runtime.write_text(data)
print('Prepared forward LE memory and explicitly single-threaded runtime')
