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
extern AudioMixer  audio_mixer[LAST_AUDIO_MIXER];

extern SDL_AudioSpec audio_spec;
extern SDL_AudioDeviceID audio_device;

void load_audio_buffers(void);
