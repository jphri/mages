#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stb_vorbis.h>
#include <stdlib.h>
#include <stdint.h>

#include "audio.h"
#include "util.h"

typedef struct {
	short *buffer;
	int length;
	int frequency;
	int channels;
} AudioBuffer;

typedef struct {
	int volume;
} AudioMixer;

typedef struct {
	const AudioBuffer *buffer;
	AudioMixer  *mixer;
	int freq_change;
	int position;
	int freq_error;
} AudioSource;

static AudioBuffer load_audio(const char *path);
static AudioBuffer load_audio_wav(const char *path);
static void source_process_callback(void *userdata, Uint8 *stream, int len);
static void process_audio_stream(Sint16 *stream, int len, AudioSource *source);

static SDL_AudioSpec     wanted_audio_spec, audio_spec;
static SDL_AudioDeviceID audio_device;

static AudioBuffer audio_buffers[LAST_AUDIO_BUFFER];
static AudioMixer  audio_mixer[LAST_AUDIO_MIXER];
static AudioSource bgm_source;
static ObjectAllocator sfx_sources;
static int bgm_playing;

#define SOURCE(ALLOC, ID) ((AudioSource*)objalloc_data(&ALLOC, ID))

void
audio_init()
{
	wanted_audio_spec.freq     = 48000;
	wanted_audio_spec.format   = AUDIO_S16SYS;
	wanted_audio_spec.channels = 2;
	wanted_audio_spec.samples  = 1024;
	wanted_audio_spec.callback = source_process_callback;

	audio_buffers[AUDIO_BUFFER_BGM_TEST] = load_audio("sounds/bgm/time-to-reflect-what-i-have-done.ogg");
	audio_buffers[AUDIO_BUFFER_FIREBALL] = load_audio_wav("sounds/sfx/fireball.wav");
	audio_buffers[AUDIO_BUFFER_FIREBALL_HIT] = load_audio_wav("sounds/sfx/fireball-hit.wav");

	audio_device = SDL_OpenAudioDevice(NULL, false, &wanted_audio_spec, &audio_spec,
		SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
	SDL_PauseAudioDevice(audio_device, 0);

	objalloc_init(&sfx_sources, sizeof(AudioSource));

	printf("Audio Initialized!\n");
	printf("Audio frequency: %d\n"
		"Audio format: %d\n"
		"Audio channels: %d\n"
		"Audio samples: %d\n",
		audio_spec.freq,
	    audio_spec.format,
	    audio_spec.channels,
	    audio_spec.samples);

	audio_mixer[AUDIO_MIXER_BGM].volume = 128;
	audio_mixer[AUDIO_MIXER_SFX].volume = 128;
}

void
audio_end()
{
	SDL_PauseAudioDevice(audio_device, 1);
	SDL_CloseAudioDevice(audio_device);
}

void
audio_sfx_play(Mixer mixer, Sound sound, float freq)
{
	ObjectID id;
	SDL_LockAudioDevice(audio_device);
	id = objalloc_alloc(&sfx_sources);
	SOURCE(sfx_sources, id)->buffer = &audio_buffers[sound];
	SOURCE(sfx_sources, id)->mixer = &audio_mixer[mixer];
	SOURCE(sfx_sources, id)->position = 0;
	SOURCE(sfx_sources, id)->freq_error = 0;
	SOURCE(sfx_sources, id)->freq_change = (int)(freq * 256);
	SDL_UnlockAudioDevice(audio_device);
}

void
source_process_callback(void *userdata, Uint8 *stream_raw, int len)
{
	(void)userdata;
	Sint16 *stream = (Sint16*)stream_raw;
	int stream_len = len / sizeof(Sint16);

	memset(stream, 0, len);
	if(bgm_playing) {
		process_audio_stream(stream, stream_len, &bgm_source);
		if(bgm_source.position >= bgm_source.buffer->length) {
			bgm_source.position = 0;
			bgm_source.freq_error = 0;
		}
	}

	for(ObjectID id = objalloc_begin(&sfx_sources); id; id = objalloc_next(&sfx_sources, id)) {
		process_audio_stream(stream, stream_len, SOURCE(sfx_sources, id));
		if(SOURCE(sfx_sources, id)->position > SOURCE(sfx_sources, id)->buffer->length)
			objalloc_free(&sfx_sources, id);
	}
	objalloc_clean(&sfx_sources);
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

void
process_audio_stream(Sint16 *stream, int stream_len, AudioSource *source)
{
	const int max = (1 << 15) - 1;
	const int min = -(1 << 15);

	for(int ll = 0; ll < stream_len; ll += 2) {
		Sint32 a1, a2, channel;
		if(source->position >= source->buffer->length)
			break;

		channel = source->buffer->channels;
		a1 = stream[ll + 0] + ((source->buffer->buffer[source->position +            0] * source->mixer->volume) >> 7);
		a2 = stream[ll + 1] + ((source->buffer->buffer[source->position +  channel - 1] * source->mixer->volume) >> 7);
		stream[ll + 0] = clampi(a1, min, max);
		stream[ll + 1] = clampi(a2, min, max);

		source->freq_error += (source->buffer->frequency * source->freq_change) >> 8;
		while(source->freq_error > audio_spec.freq) {
			source->freq_error -= audio_spec.freq;
			source->position += source->buffer->channels;
		}
	}
}

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
audio_bgm_pause()
{
	bgm_playing = 0;
}

void
audio_bgm_resume()
{
	bgm_playing = 1;
}

void
audio_set_volumer(Mixer mixer, float v)
{
	audio_mixer[mixer].volume = 128 * v;
}