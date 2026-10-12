#ifndef OPENFORMULA_AUDIO_AHI_H
#define OPENFORMULA_AUDIO_AHI_H

int of_audio_start(void);
void of_audio_stop(void);
int of_audio_available(void);
void of_audio_set_engine(unsigned rpm, unsigned throttle, unsigned speed, int steer);
void of_audio_beep(unsigned kind);
void of_audio_hit(void);

#endif
