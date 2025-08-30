#include <SDL.h>
#include <stdbool.h>
#include <stb_vorbis.h>
#include <stdint.h>
#include <audio.h>

#include "dat.h"
#include "fns.h"

static void source_process_callback(void *userdata, Uint8 *stream, int len);

static SDL_AudioSpec     wanted_audio_spec;

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

	init_sfx_system();
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
	unload_audio_buffers();
	SDL_PauseAudioDevice(audio_device, 1);
	SDL_CloseAudioDevice(audio_device);
}

void
source_process_callback(void *userdata, Uint8 *stream_raw, int len)
{
	(void)userdata;
	Sint16 *stream = (Sint16*)stream_raw;
	int stream_len = len / sizeof(Sint16);

	memset(stream, 0, len);
	process_bgm(stream_len, stream);
	process_sfx(stream_len, stream);
}

