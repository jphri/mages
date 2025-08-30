typedef struct {
	short *buffer;
	int length;
	int frequency;
	int channels;
} AudioBuffer;

extern AudioBuffer audio_buffers[LAST_AUDIO_BUFFER];

void load_audio_buffers(void);
