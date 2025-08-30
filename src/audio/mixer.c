#include <audio.h>
#include <SDL.h>

#include "dat.h"

AudioMixer audio_mixer[LAST_AUDIO_MIXER];

void
audio_set_volumer(Mixer mixer, float v)
{
	audio_mixer[mixer].volume = 128 * v;
}
