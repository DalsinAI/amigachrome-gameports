/*
 * OpenScope - generated SDL audio with an oscilloscope display.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>

#define VIEW_W 320
#define VIEW_H 240
#define RING_SAMPLES 512

typedef struct AudioState {
    SDL_AudioDeviceID device;
    SDL_AudioSpec have;
    float phase_a;
    float phase_b;
    float frequency;
    float volume;
    int muted;
    int waveform;
    Sint16 ring[RING_SAMPLES];
    unsigned write_pos;
} AudioState;

static float wave_sample(int waveform, float phase)
{
    if (waveform == 1) {
        return sinf(phase) >= 0.0f ? 1.0f : -1.0f;
    }
    if (waveform == 2) {
        float unit = phase / 6.28318530f;
        unit -= floorf(unit);
        return unit * 2.0f - 1.0f;
    }
    return sinf(phase);
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioState *state = (AudioState *)userdata;
    Sint16 *output = (Sint16 *)stream;
    int frames = len / ((int)sizeof(Sint16) * 2);
    int i;
    float step_a = 6.28318530f * state->frequency / (float)state->have.freq;
    float step_b = 6.28318530f * (state->frequency * 1.503f) / (float)state->have.freq;

    for (i = 0; i < frames; ++i) {
        float a = wave_sample(state->waveform, state->phase_a);
        float b = sinf(state->phase_b) * 0.36f;
        float mixed = (a * 0.62f + b) * state->volume;
        Sint16 sample;

        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        if (state->muted) mixed = 0.0f;
        sample = (Sint16)(mixed * 30000.0f);
        output[i * 2] = sample;
        output[i * 2 + 1] = sample;
        state->ring[state->write_pos & (RING_SAMPLES - 1)] = sample;
        ++state->write_pos;

        state->phase_a += step_a;
        state->phase_b += step_b;
        if (state->phase_a >= 6.28318530f) state->phase_a -= 6.28318530f;
        if (state->phase_b >= 6.28318530f) state->phase_b -= 6.28318530f;
    }
}

static int start_audio(AudioState *state)
{
    SDL_AudioSpec want;

    SDL_zero(want);
    SDL_zero(state->have);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = state;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "OpenScope: SDL audio init failed: %s\n", SDL_GetError());
        return 0;
    }

    state->device = SDL_OpenAudioDevice(NULL, 0, &want, &state->have, 0);
    if (!state->device) {
        fprintf(stderr, "OpenScope: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return 0;
    }
    if (state->have.format != AUDIO_S16SYS || state->have.channels != 2) {
        fprintf(stderr, "OpenScope: unexpected audio format 0x%04x channels=%u\n",
                state->have.format, (unsigned)state->have.channels);
        SDL_CloseAudioDevice(state->device);
        state->device = 0;
        return 0;
    }

    fprintf(stdout, "OpenScope: audio=%d Hz S16 stereo samples=%u\n",
            state->have.freq, (unsigned)state->have.samples);
    fflush(stdout);
    SDL_PauseAudioDevice(state->device, 0);
    return 1;
}

int main(int argc, char **argv)
{
    DemoApp app;
    AudioState audio;
    Sint16 snapshot[RING_SAMPLES];
    SDL_Point waveform[VIEW_W];
    SDL_Event event;
    int running = 1;
    int paused = 0;
    int audio_ok;
    int i;
    (void)argc;
    (void)argv;

    SDL_zero(audio);
    audio.frequency = 220.0f;
    audio.volume = 0.28f;

    if (!demo_init(&app, "OpenScope", VIEW_W, VIEW_H, 2)) {
        return 1;
    }

    audio_ok = start_audio(&audio);

    while (running) {
        char mode[128];
        const char *wave_name = audio.waveform == 0 ? "sine" :
                                audio.waveform == 1 ? "square" : "saw";

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                int lock = audio_ok && audio.device;
                if (lock) SDL_LockAudioDevice(audio.device);
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_UP:
                    case SDLK_RIGHT:
                        audio.frequency *= 1.0594631f;
                        if (audio.frequency > 1760.0f) audio.frequency = 1760.0f;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        audio.frequency /= 1.0594631f;
                        if (audio.frequency < 55.0f) audio.frequency = 55.0f;
                        break;
                    case SDLK_m:
                        audio.muted = !audio.muted;
                        break;
                    case SDLK_w:
                        audio.waveform = (audio.waveform + 1) % 3;
                        break;
                    default:
                        break;
                }
                if (lock) SDL_UnlockAudioDevice(audio.device);
                if (audio_ok && audio.device && event.key.keysym.sym == SDLK_SPACE) {
                    SDL_PauseAudioDevice(audio.device, paused ? 1 : 0);
                }
            }
        }

        memset(snapshot, 0, sizeof(snapshot));
        if (audio_ok && audio.device) {
            unsigned pos;
            SDL_LockAudioDevice(audio.device);
            pos = audio.write_pos;
            for (i = 0; i < RING_SAMPLES; ++i) {
                snapshot[i] = audio.ring[(pos + i) & (RING_SAMPLES - 1)];
            }
            wave_name = audio.waveform == 0 ? "sine" :
                        audio.waveform == 1 ? "square" : "saw";
            SDL_UnlockAudioDevice(audio.device);
        }

        SDL_SetRenderDrawColor(app.renderer, 2, 8, 8, 255);
        SDL_RenderClear(app.renderer);
        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app.renderer, 20, 90, 72, 95);
        for (i = 0; i <= VIEW_W; i += 20) {
            SDL_RenderDrawLine(app.renderer, i, 0, i, VIEW_H - 1);
        }
        for (i = 0; i <= VIEW_H; i += 20) {
            SDL_RenderDrawLine(app.renderer, 0, i, VIEW_W - 1, i);
        }
        SDL_SetRenderDrawColor(app.renderer, 80, 255, 170, 230);
        SDL_RenderDrawLine(app.renderer, 0, VIEW_H / 2, VIEW_W - 1, VIEW_H / 2);

        for (i = 0; i < VIEW_W; ++i) {
            int index = i * RING_SAMPLES / VIEW_W;
            int y = VIEW_H / 2 - (snapshot[index] * 82 / 32768);
            waveform[i].x = i;
            waveform[i].y = y;
        }
        SDL_SetRenderDrawColor(app.renderer, 110, 255, 220, 255);
        SDL_RenderDrawLines(app.renderer, waveform, VIEW_W);

        for (i = 0; i < 32; ++i) {
            int start = i * (RING_SAMPLES / 32);
            int j;
            int peak = 0;
            SDL_Rect bar;
            for (j = 0; j < RING_SAMPLES / 32; ++j) {
                int value = snapshot[start + j];
                if (value < 0) value = -value;
                if (value > peak) peak = value;
            }
            bar.x = i * 10 + 2;
            bar.w = 6;
            bar.h = peak * 52 / 32768;
            bar.y = VIEW_H - 10 - bar.h;
            SDL_SetRenderDrawColor(app.renderer,
                                   (Uint8)(40 + i * 5),
                                   (Uint8)(180 + i * 2),
                                   (Uint8)(220 - i * 3),
                                   190);
            SDL_RenderFillRect(app.renderer, &bar);
        }

        SDL_RenderPresent(app.renderer);
        if (audio_ok) {
            snprintf(mode, sizeof(mode), "audio %dHz %.1fHz %s %s%s - arrows/W/M/Space",
                     audio.have.freq,
                     audio.frequency,
                     wave_name,
                     audio.muted ? "muted" : "playing",
                     paused ? " paused" : "");
        } else {
            snprintf(mode, sizeof(mode), "audio unavailable - renderer display only");
        }
        demo_title_tick(&app, mode);
    }

    if (audio.device) {
        SDL_PauseAudioDevice(audio.device, 1);
        SDL_CloseAudioDevice(audio.device);
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    demo_shutdown(&app);
    return 0;
}
