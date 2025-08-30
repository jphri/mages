#include <SDL.h>
#include <stdbool.h>
#include <stb_vorbis.h>
#include <stdlib.h>
#include <stdint.h>

#include "audio.h"
#include "util.h"

#include "dat.h"
#include "fns.h"

static void source_process_callback(void *userdata, Uint8 *stream, int len);

static SDL_AudioSpec     wanted_audio_spec;
static ObjectPool sfx_sources;

SDL_AudioSpec audio_spec;
SDL_AudioDeviceID audio_device;

void
audio_init(void)
{
	wanted_audio_spec.freq     = 48000;
	wanted_audio_spec.format   = AUDIO_S16SYS;
	wanted_audio_spec.channels = 2;
	wanted_audio_spec.samples  = 1024;
	wanted_audio_spec.callback = source_process_callback;

	audio_device = SDL_OpenAudioDevice(NULL, false, &wanted_audio_spec, &audio_spec,
		SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
	SDL_PauseAudioDevice(audio_device, 0);

	objpool_init(&sfx_sources, sizeof(AudioSource), DEFAULT_ALIGNMENT);
	load_audio_buffers();

	printf("Audio Initialized!\n");
	printf("Audio frequency: %d\n"
		"Audio format: %d\n"
		"Audio channels: %d\n"
		"Audio samples: %d\n",
		audio_spec.freq,
	    audio_spec.format,
	    audio_spec.channels,
	    audio_spec.samples);

	audio_set_volumer(AUDIO_MIXER_BGM, 1.0);
	audio_set_volumer(AUDIO_MIXER_SFX, 1.0);
}

void
audio_end(void)
{
	SDL_PauseAudioDevice(audio_device, 1);
	SDL_CloseAudioDevice(audio_device);

	for(int i = 0; i < LAST_AUDIO_BUFFER; i++) {
		free(audio_buffers[i].buffer);
	}

}

void
audio_sfx_play(Mixer mixer, Sound sound, float freq)
{
	SDL_LockAudioDevice(audio_device);
	AudioSource *source = objpool_new(&sfx_sources);
	source->buffer = &audio_buffers[sound];
	source->mixer = &audio_mixer[mixer];
	source->position = 0;
	source->freq_error = 0;
	source->freq_change = (int)(freq * 256);
	SDL_UnlockAudioDevice(audio_device);
}

void
source_process_callback(void *userdata, Uint8 *stream_raw, int len)
{
	(void)userdata;
	Sint16 *stream = (Sint16*)stream_raw;
	int stream_len = len / sizeof(Sint16);

	memset(stream, 0, len);
	process_bgm(stream_len, stream);

	for(AudioSource *source = objpool_begin(&sfx_sources); source; source = objpool_next(source)) {
		process_audio_stream(stream, stream_len, source);
		if(source->position > source->buffer->length)
			objpool_free(source);
	}
	objpool_clean(&sfx_sources);
}

