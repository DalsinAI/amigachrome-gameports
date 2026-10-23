/* Host capability probe; dummy output is not an AHI runtime qualification. */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

static SDL_AtomicInt matches;
static SDL_AtomicInt phase;

static void SDLCALL observe(void *userdata, const SDL_AudioSpec *spec, float *buffer, int bytes)
{
    (void)userdata;
    const float expected = SDL_GetAtomicInt(&phase) ? 0.125f : 0.375f;
    if (spec->format != SDL_AUDIO_F32 || spec->channels != 2) return;
    for (int i = 0; i < bytes / (int)sizeof(float); ++i) {
        if (fabsf(buffer[i] - expected) < 0.0005f) {
            SDL_AddAtomicInt(&matches, 1);
        }
    }
}

static int wait_for_mix(void)
{
    const Uint64 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < 2000) {
        if (SDL_GetAtomicInt(&matches) > 256) return 1;
        SDL_Delay(5);
    }
    return 0;
}

int main(void)
{
    float pcm[48000];
    for (int i = 0; i < 48000; ++i) pcm[i] = 0.5f;
    if (!SDL_Init(SDL_INIT_AUDIO)) goto failed;
    SDL_AudioSpec input = {SDL_AUDIO_F32, 1, 48000};
    SDL_AudioSpec output = {SDL_AUDIO_F32, 2, 48000};
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &output);
    if (!device || !SDL_PauseAudioDevice(device)) goto failed;
    SDL_AudioStream *streams[2] = {
        SDL_CreateAudioStream(&input, &output),
        SDL_CreateAudioStream(&input, &output)
    };
    if (!streams[0] || !streams[1]) goto failed;
    if (!SDL_SetAudioStreamGain(streams[0], 0.5f)
        || !SDL_SetAudioStreamGain(streams[1], 0.25f)
        || !SDL_SetAudioStreamFrequencyRatio(streams[0], 1.25f)
        || fabsf(SDL_GetAudioStreamFrequencyRatio(streams[0]) - 1.25f) > 0.00001f
        || !SDL_PutAudioStreamData(streams[0], pcm, sizeof(pcm))
        || !SDL_PutAudioStreamData(streams[1], pcm, sizeof(pcm))
        || !SDL_BindAudioStreams(device, streams, 2)
        || !SDL_SetAudioPostmixCallback(device, observe, NULL)
        || !SDL_ResumeAudioDevice(device)) goto failed;
    if (!wait_for_mix()) goto failed;
    SDL_UnbindAudioStream(streams[0]);
    SDL_SetAtomicInt(&phase, 1);
    SDL_SetAtomicInt(&matches, 0);
    if (!wait_for_mix()) goto failed;
    SDL_DestroyAudioStream(streams[0]);
    SDL_DestroyAudioStream(streams[1]);
    SDL_CloseAudioDevice(device);
    SDL_Quit();
    puts("PASS: two streams mix with independent gains, pitch setting, mono/stereo conversion, unbind and clean close (dummy output)");
    return 0;
failed:
    fprintf(stderr, "FAIL: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
}
