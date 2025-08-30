#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
	AUDIO_BUFFER_NULL,
	AUDIO_BUFFER_BGM_TEST,
	AUDIO_BUFFER_FIREBALL,
	AUDIO_BUFFER_FIREBALL_HIT,
	AUDIO_BUFFER_DOOR_OPEN,
	AUDIO_BUFFER_DOOR_CLOSE,
	LAST_AUDIO_BUFFER
} Sound;

typedef enum {
	AUDIO_MIXER_ALL,
	AUDIO_MIXER_BGM,
	AUDIO_MIXER_SFX,
	LAST_AUDIO_MIXER
} Mixer;

void audio_init(void);
void audio_bgm_play(Sound sound, float freq_scale);
void audio_bgm_pause(void);
void audio_bgm_resume(void);
void audio_sfx_play(Mixer mixer, Sound sound, float freq_scale);
void audio_end(void);

void audio_set_volumer(Mixer mixer, float v);

#endif
