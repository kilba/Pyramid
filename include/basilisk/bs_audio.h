#ifndef BS_AUDIO_H
#define BS_AUDIO_H

void bs_playSound(bs_Sound* sound, float volume);
bs_Sound bs_sound(const char* name);
void bs_initAudio();

#endif // BS_AUDIO_H