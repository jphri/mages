#include <SDL_audio.h>

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

extern AudioBuffer audio_buffers[LAST_AUDIO_BUFFER];
extern SDL_AudioSpec audio_spec;

void load_audio_buffers(void);
