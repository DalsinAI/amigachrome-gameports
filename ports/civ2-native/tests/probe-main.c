#include <stdio.h>
#include <string.h>
#include "memory-probe.h"
#include "wasm-rt-impl.h"
#include "wasm-rt-exceptions.h"

#define CHECK(test) do { if (!(test)) { \
  printf("FAIL line %d: %s\n", __LINE__, #test); return 1; } } while (0)

int main(void) {
  w2c_probe module;
  wasm_rt_init();
  wasm2c_probe_instantiate(&module);
  wasm_rt_memory_t* memory = w2c_probe_memory(&module);
  CHECK(strcmp((const char*)memory->data + 16, "AmigaChrome native 68k") == 0);
  w2c_probe_store32(&module, 101, 0x12345678u);
  CHECK(memcmp(memory->data + 101, "\x78\x56\x34\x12", 4) == 0);
  CHECK(w2c_probe_load32(&module, 101) == 0x12345678u);
  CHECK(w2c_probe_load16(&module, 102) == 0x3456u);
  CHECK(w2c_probe_load8(&module, 104) == 0x12u);
  memcpy(memory->data + 201, "\xef\xcd\xab\x89", 4);
  CHECK(w2c_probe_load32(&module, 201) == 0x89abcdefu);
  w2c_probe_store64(&module, 301, UINT64_C(0x0123456789abcdef));
  CHECK(memcmp(memory->data + 301, "\xef\xcd\xab\x89\x67\x45\x23\x01", 8) == 0);
  CHECK(w2c_probe_load64(&module, 301) == UINT64_C(0x0123456789abcdef));
  w2c_probe_storefloat(&module, 401, 1.5f);
  CHECK(memcmp(memory->data + 401, "\x00\x00\xc0\x3f", 4) == 0);
  CHECK(w2c_probe_loadfloat(&module, 401) == 1.5f);
  w2c_probe_copy(&module, 501, 16, 22);
  CHECK(strcmp((const char*)memory->data + 501, "AmigaChrome native 68k") == 0);
  CHECK(w2c_probe_grow(&module) == 1);
  CHECK(memory->size == 131072);
  CHECK(w2c_probe_load32(&module, 101) == 0x12345678u);
  CHECK(strcmp((const char*)memory->data + 16, "AmigaChrome native 68k") == 0);
  wasm_rt_trap_t code = wasm_rt_impl_try();
  if (code == WASM_RT_TRAP_NONE) {
    (void)w2c_probe_load32(&module, 131071);
    CHECK(0);
  }
  CHECK(code == WASM_RT_TRAP_OOB);
  wasm2c_probe_free(&module);
  wasm_rt_free();
  puts("PASS wasm2c native: unaligned LE integers, float, host strings, copy, grow, bounds trap, teardown");
  return 0;
}
