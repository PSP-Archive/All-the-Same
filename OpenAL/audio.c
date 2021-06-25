
/*
 *
 * audio.c - Audio related functions
 */

#define WIN32_LEAN_AND_MEAN		// exclude rarely-used stuff from windows headers

#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#endif

#ifdef LINUX
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#endif

#ifdef SDL
#include <SDL.h>
#include <SDL_thread.h>
#endif

#include <alut.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "common.h"
#include "audio.h"

// ---------------------------------------------------------------------------
// definitions
// ---------------------------------------------------------------------------

// the number of audio source we'll have
#define AUDIO_SOURCES 8

// OGG decoder buffer size
#define OGG_BUFFER_SIZE (4096 * 32)

// ---------------------------------------------------------------------------
// variables (GLOBAL)
// ---------------------------------------------------------------------------

// did we succeed loading the BGM?
int audioBGMLoaded = NO;

// play BGM?
int audioBGM = YES;

// play SFX?
int audioSFX = YES;

// ---------------------------------------------------------------------------
// variables (INTERNAL)
// ---------------------------------------------------------------------------

// the BGMs
char **bgms = NULL;

// the number of BGMs we have
int bgmsN = 0;

// the SFXs
struct sfx *sfxs = NULL;

// the number of SFXs we have
int sfxsN = 0;

// the last played BGM
int audioBGMLast = -1;

// the next source ID we'll use for playing (round robin)
int audioSFXNextSource;

// the OpenAL audio buffer IDs
ALuint audioSFXBufferIDs[AUDIO_SAMPLES_MAX];

// the OpenAL audio source IDs
ALuint audioSFXSourceIDs[AUDIO_SOURCES];

// the OGG VORBIS file
OggVorbis_File oggFileVorbis;

// the OGG file
FILE *oggFile;

// double buffers for OGG
ALuint oggBuffers[2] = { -1, -1 };

// the OGG sound source
ALuint oggSource = -1;

// the OGG info
vorbis_info *oggInfo;

// the OFF format
ALenum oggFormat;

// ---------------------------------------------------------------------------
// play the supplied SFX
// ---------------------------------------------------------------------------

void audioPlaySFX(int sfx) {

	if (audioSFX == YES) {
	  alSourcei(audioSFXSourceIDs[audioSFXNextSource], AL_BUFFER, audioSFXBufferIDs[sfx]);
		alSourcePlay(audioSFXSourceIDs[audioSFXNextSource]);

		audioSFXNextSource++;
		if (audioSFXNextSource == AUDIO_SOURCES)
			audioSFXNextSource = 0;
	}
}

// ---------------------------------------------------------------------------
// init the audio subsystem
// ---------------------------------------------------------------------------

void audioInit(void) {

	if (alutInit(NULL, NULL) == 0)
		return;

	// create sources
	alGenSources(AUDIO_SOURCES, audioSFXSourceIDs);

	audioSFXNextSource = 0;
}

// ---------------------------------------------------------------------------
// specify the BGMs
// ---------------------------------------------------------------------------

void audioSetBGMList(char **bgmList, int bgmN) {

	bgms = bgmList;
	bgmsN = bgmN;
}

// ---------------------------------------------------------------------------
// specify the SFXs
// ---------------------------------------------------------------------------

void audioSetSFXList(struct sfx *sfxList, int sfxN) {

	int i;

	sfxs = sfxList;
	sfxsN = sfxN;

	// load the SFX samples
	for (i = 0; i < sfxN; i++) {
		audioSFXBufferIDs[i] = alutCreateBufferFromFile(sfxList[i].name);
		if (audioSFXBufferIDs[i] == AL_NONE)
			;
	}
}

// ---------------------------------------------------------------------------
// OGG
// ---------------------------------------------------------------------------

int oggFillBuffer(ALuint buffer) {

	char data[OGG_BUFFER_SIZE];
	int size = 0;
	int section;
	int result;

	while (size < OGG_BUFFER_SIZE) {
		result = ov_read(&oggFileVorbis, data + size, OGG_BUFFER_SIZE - size, 0, 2, 1, &section);
		if (result > 0)
			size += result;
		else
			break;
	}

	if (size == 0)
		return NO;

	alBufferData(buffer, oggFormat, data, size, oggInfo->rate);

	return NO;
}

#ifdef SDL

// quit the thread?
int oggThreadQuit;

// the thread
SDL_Thread *oggThread = NULL;

int oggPlayerThread(void *unused) {

	while (oggThreadQuit == NO) {
		// update the BGM, perhaps decode more audio into buffers
		audioUpdateBGM();

		SDL_Delay(10);
	}

	return 0;
}

#endif

int oggUpdate(void) {

	int processed;
	int active = YES;
	ALuint buffer;

	alGetSourcei(oggSource, AL_BUFFERS_PROCESSED, &processed);

	while (processed--) {
		alSourceUnqueueBuffers(oggSource, 1, &buffer);
		active = oggFillBuffer(buffer);
		alSourceQueueBuffers(oggSource, 1, &buffer);

		// start playing
		alSourcePlay(oggSource);
	}

	return active;
}

void audioUpdateBGM(void) {

	ALenum state;

	while (oggUpdate() == NO) {
		alGetSourcei(oggSource, AL_SOURCE_STATE, &state);

		if (state != AL_PLAYING) {
			oggFillBuffer(oggBuffers[0]);
			oggFillBuffer(oggBuffers[1]);

			alSourceQueueBuffers(oggSource, 2, oggBuffers);
			alSourcePlay(oggSource);
		}
	}
}

// ---------------------------------------------------------------------------
// play the supplied BGM
// ---------------------------------------------------------------------------

void audioPlayBGM(int bgm) {

	if (audioBGMLoaded == YES) {
		// end the old thread
		oggThreadQuit = YES;

#ifdef SDL
		if (oggThread != NULL) {
			SDL_WaitThread(oggThread, NULL);
			oggThread = NULL;
		}
#endif
	}

	audioBGMLoaded = NO;

	// delete the previous player
	if (oggSource >= 0) {
		alDeleteSources(1, &oggSource);
		oggSource = -1;
	}
	if (oggBuffers[0] >= 0) {
		alDeleteBuffers(2, oggBuffers);
		oggBuffers[0] = -1;
		oggBuffers[1] = -1;
	}

	if (bgm < 0)
		return;

	// generate OGG sources and buffers
	alGenBuffers(2, oggBuffers);
	alGenSources(1, &oggSource);
	alSource3f(oggSource, AL_POSITION,  0.0, 0.0, 0.0);
	alSource3f(oggSource, AL_VELOCITY,  0.0, 0.0, 0.0);
	alSource3f(oggSource, AL_DIRECTION, 0.0, 0.0, 0.0);
	alSourcef(oggSource, AL_ROLLOFF_FACTOR, 0.0);
	alSourcei(oggSource, AL_SOURCE_RELATIVE, AL_TRUE);

	audioBGMLast = bgm;

	oggFile = fopen(bgms[bgm], "rb");
	if (oggFile == NULL)
		return;
	if (ov_open(oggFile, &oggFileVorbis, NULL, 0) < 0) {
		fclose(oggFile);
		return;
	}

	oggInfo = ov_info(&oggFileVorbis, -1);
	if (oggInfo->channels == 1)
		oggFormat = AL_FORMAT_MONO16;
	else
		oggFormat = AL_FORMAT_STEREO16;

	// fill the buffers
	oggFillBuffer(oggBuffers[0]);
	oggFillBuffer(oggBuffers[1]);

	// start playing
	alSourceQueueBuffers(oggSource, 2, oggBuffers);
	alSourcePlay(oggSource);

	// the player thread
	oggThreadQuit = NO;

#ifdef SDL
	oggThread = SDL_CreateThread(oggPlayerThread, NULL);
#endif

	audioBGMLoaded = YES;
}

// ---------------------------------------------------------------------------
// set the status of SFX
// ---------------------------------------------------------------------------

void audioSetSFX(int status) {

	audioSFX = status;
}

// ---------------------------------------------------------------------------
// set the status of BGM
// ---------------------------------------------------------------------------

void audioSetBGM(int status) {

	audioBGM = status;

	if (status == YES)
		audioPlayBGM(audioBGMLast);
	else
		audioPlayBGM(-1);
}
