
#ifndef AUDIO_H
#define AUDIO_H

// the maximum number of samples the game has
#define AUDIO_SAMPLES_MAX 16

// use these IDs to play nothing
#define SFX_NONE -1
#define BGM_NONE -1

struct sfx {
	char *name;          // name of the sample file
	unsigned char *data; // 16bit, mono, 44100Hz
	int length;          // the sample length
};

// did we succeed loading the BGM?
extern int audioBGMLoaded;

// play BGM?
extern int audioBGM;

// play SFX?
extern int audioSFX;

void audioInit(void);
void audioSetBGMList(char **bgmList, int bgmN);
void audioSetSFXList(struct sfx *sfxList, int sfxN);

void audioPlaySFX(int sfx);
void audioPlayBGM(int bgm);
void audioSetBGM(int status);
void audioSetSFX(int status);
void audioUpdateBGM(void);

#endif
