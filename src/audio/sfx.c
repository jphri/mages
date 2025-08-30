#include <audio.h>
#include <SDL.h>
#include <util.h>

#include "dat.h"
#include "fns.h"

static ObjectPool sfx_sources;

void
init_sfx_system(void)
{
	objpool_init(&sfx_sources, sizeof(AudioSource), DEFAULT_ALIGNMENT);
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
process_sfx(int stream_len, Sint16 stream[stream_len])
{
	for(AudioSource *source = objpool_begin(&sfx_sources); source; source = objpool_next(source)) {
		process_audio_stream(stream, stream_len, source);
		if(source->position > source->buffer->length)
			objpool_free(source);
	}
	objpool_clean(&sfx_sources);
}
