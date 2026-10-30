/* SPDX-License-Identifier: GPL-3.0-or-later
 * Force-included (-include) into native/audio_service.cpp only, in the
 * WebAssembly build. Emscripten's SDL2 without pthreads has no threads, so
 * the audio service's music feeder thread (feed(): pump, SDL_Delay(30),
 * repeat while running) could not start and music would never stream.
 *
 * These four names are redirected to amiga/wasm_runtime.cpp, which runs the
 * thread function cooperatively: one loop iteration (one music_pump) each
 * time the browser build yields (WaitPort, GetMsg, the engine's yield point),
 * at most every 20 ms. Outside that tick the calls behave as SDL's own.
 * SDL.h is included first so SDL's declarations keep their real names.
 */
#ifndef ALPHA_WASM_SDL_COOP_THREAD_H
#define ALPHA_WASM_SDL_COOP_THREAD_H
#include <SDL2/SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
SDL_Thread* alpha_coop_create_thread(SDL_ThreadFunction fn, const char* name, void* data);
void alpha_coop_wait_thread(SDL_Thread* thread, int* status);
int alpha_coop_atomic_get(SDL_atomic_t* value);
void alpha_coop_delay(Uint32 ms);
#ifdef __cplusplus
}
#endif
#define SDL_CreateThread alpha_coop_create_thread
#define SDL_WaitThread alpha_coop_wait_thread
#define SDL_AtomicGet alpha_coop_atomic_get
#define SDL_Delay alpha_coop_delay
#endif
