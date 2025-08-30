#include <SDL.h>
#include <audio.h>

#include "dat.h"
#include "fns.h"

static AudioSource bgm_source;
static int bgm_playing;

void
audio_bgm_play(Sound sound, float freq_scale)
{
	SDL_LockAudioDevice(audio_device);
	bgm_source.buffer = &audio_buffers[sound];
	bgm_source.mixer = &audio_mixer[AUDIO_MIXER_BGM];
	bgm_source.position = 0;
	bgm_source.freq_error = 0;
	bgm_source.freq_change = (int)(freq_scale * 256);
	SDL_UnlockAudioDevice(audio_device);
	bgm_playing = 1;
}

void
audio_bgm_pause(void)
{
	bgm_playing = 0;
}

void
audio_bgm_resume(void)
{
	bgm_playing = 1;
}

void
process_bgm(int len, Sint16 stream[len])
{
	if(bgm_playing) {
		process_audio_stream(stream, len, &bgm_source);
		if(bgm_source.position >= bgm_source.buffer->length) {
			bgm_source.position = 0;
			bgm_source.freq_error = 0;
		}
	}
}
