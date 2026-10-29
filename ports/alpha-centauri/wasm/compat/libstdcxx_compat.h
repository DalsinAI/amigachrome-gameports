/* SPDX-License-Identifier: GPL-3.0-or-later
 * Force-included (-include) into every WebAssembly compile. Emscripten ships
 * libc++, the native builds use GCC's libstdc++; the engine (Thinker's
 * combat.cpp) calls libstdc++'s std::__gcd. This is libstdc++'s Euclid loop
 * (sign behaviour included), so results match the native builds exactly.
 */
#ifndef ALPHA_WASM_LIBSTDCXX_COMPAT_H
#define ALPHA_WASM_LIBSTDCXX_COMPAT_H
#ifdef __cplusplus
namespace std {
template <class T> inline T __gcd(T m, T n) { while (n != 0) { T t = m % n; m = n; n = t; } return m; }
}
#endif
#endif
