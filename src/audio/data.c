#include <util.h>
#include <SDL.h>
#include <audio.h>

#include <stb_vorbis.h>

#include "dat.h"

static AudioBuffer load_audio(const char *path);
static AudioBuffer load_audio_wav(const char *path);

AudioBuffer audio_buffers[LAST_AUDIO_BUFFER];

void
load_audio_buffers(void)
{
	audio_buffers[AUDIO_BUFFER_BGM_TEST] = load_audio("sounds/bgm/time-to-reflect-what-i-have-done.ogg");
	audio_buffers[AUDIO_BUFFER_FIREBALL] = load_audio_wav("sounds/sfx/fireball.wav");
	audio_buffers[AUDIO_BUFFER_FIREBALL_HIT] = load_audio_wav("sounds/sfx/fireball-hit.wav");
	audio_buffers[AUDIO_BUFFER_DOOR_OPEN]  = load_audio_wav("sounds/sfx/door-open.wav");
	audio_buffers[AUDIO_BUFFER_DOOR_CLOSE] = load_audio_wav("sounds/sfx/door-close.wav");
}

AudioBuffer
load_audio(const char *path)
{
	AudioBuffer buffer = {0};
	buffer.length = stb_vorbis_decode_filename(path, &buffer.channels, &buffer.frequency, &buffer.buffer);
	return buffer;
}

AudioBuffer
load_audio_wav(const char *path)
{
	AudioBuffer buffer;
	
	SDL_AudioSpec spec;
	Uint32 wav_length;
	Uint8 *wav_buffer;

	if(SDL_LoadWAV(path, &spec, &wav_buffer, &wav_length) == NULL) {
		die("cannot load file: %s\n", path);
	}
	
	buffer.channels = spec.channels;
	buffer.frequency = spec.freq;
	buffer.length = wav_length / sizeof(Sint16);;
	buffer.buffer = (short*)wav_buffer;

	return buffer;
}
