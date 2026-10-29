/* SPDX-License-Identifier: GPL-3.0-or-later
 * WebAssembly build only. original_ui_text.h forward-declares
 * `struct TTF_Font` (the SDK SDL2_ttf's tag); emscripten's SDL2_ttf 2.20 port
 * names it `struct _TTF_Font`. Map the tag so both declarations agree. The
 * API is extern "C", so the tag does not reach any symbol name.
 */
#ifndef ALPHA_WASM_SDL_TTF_WRAPPER_H
#define ALPHA_WASM_SDL_TTF_WRAPPER_H
#define _TTF_Font TTF_Font
#include_next <SDL2/SDL_ttf.h>
#undef _TTF_Font
#endif
