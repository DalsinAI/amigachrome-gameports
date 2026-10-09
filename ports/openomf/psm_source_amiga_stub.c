#include "audio/music_sources/psm_source.h"

unsigned
psm_get_resamplers(const music_resampler **resamplers)
{
    if (resamplers != NULL)
        *resamplers = NULL;
    return 0;
}

bool
psm_load(music_source *src, int channels, int sample_rate, int resampler, const char *file)
{
    (void)src;
    (void)channels;
    (void)sample_rate;
    (void)resampler;
    (void)file;
    return false;
}
