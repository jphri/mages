#include <SDL.h>
#include <audio.h>

#include "dat.h"
#include "util.h"

void
process_audio_stream(Sint16 *stream, int stream_len, AudioSource *source)
{
	const int max = (1 << 15) - 1;
	const int min = -(1 << 15);

	for(int ll = 0; ll < stream_len; ll += audio_spec.channels) {
		Sint32 channel;
		if(source->position >= source->buffer->length)
			break;

		channel = source->buffer->channels;
		for(int i = 0; i < audio_spec.channels; i++) {
			int sample = source->buffer->buffer[source->position + (i % channel)];
			int a = stream[ll + i] + ((sample * source->mixer->volume) >> 7);
			stream[ll + i] = clampi(a, min, max);
		}

		source->freq_error += (source->buffer->frequency * source->freq_change) >> 8;
		while(source->freq_error > audio_spec.freq) {
			source->freq_error -= audio_spec.freq;
			source->position += source->buffer->channels;
		}
	}
}
