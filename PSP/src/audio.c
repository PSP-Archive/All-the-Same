
/*
 *
 * audio.c - Audio related functions
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspaudio.h>
#include <pspaudiolib.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "common.h"
#include "audio.h"

#include "mp3player.h"

// ---------------------------------------------------------------------------
// definitions
// ---------------------------------------------------------------------------

struct sfxChannel {
	int sfxID;    // the SFX we are currently playing on this channel
	int position; // the position in the SFX
};

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

// the SFX channels
struct sfxChannel sfxChannels[4*2];

// the SFXs
extern unsigned char sfxMenuClick_start[];

// ---------------------------------------------------------------------------
// the mixer callback
// ---------------------------------------------------------------------------

void audioSFXCallbackMixer(void *buf, unsigned int length, int sfxChannel) {

	short *out = (short *)buf;
	short *left, *right;
	int i, lengthLeft = 0, lengthRight = 0, *positionLeft = NULL, *positionRight = NULL;

	// set up the channel pointers
	i = sfxChannels[sfxChannel].sfxID;
	if (i == SFX_NONE)
		left = NULL;
	else {
		left = (short *)sfxs[i].data;
		lengthLeft = sfxs[i].length;
		positionLeft = &sfxChannels[sfxChannel].position;
	}

	i = sfxChannels[sfxChannel + 1].sfxID;
	if (i == SFX_NONE)
		right = NULL;
	else {
		right = (short *)sfxs[i].data;
		lengthRight = sfxs[i].length;
		positionRight = &sfxChannels[sfxChannel + 1].position;
	}

	// 16bit, stereo, 44100Hz
	i = 0;

	if (left == NULL && right == NULL) {
		// nothing to mix!
		unsigned int *o = (unsigned int *)out;

		while (i < length) {
			o[i++] = 0;
			o[i++] = 0;
		}
	}
	else {
		// mix the SFX
		length <<= 1;
		while (i < length) {
			// left
			if (left == NULL)
				out[i++] = 0;
			else {
				out[i++] = left[(*positionLeft)++];
				if (*positionLeft == lengthLeft) {
					left = NULL;
					sfxChannels[sfxChannel].sfxID = SFX_NONE;
				}
			}

			// right
			if (right == NULL)
				out[i++] = 0;
			else {
				out[i++] = right[(*positionRight)++];
				if (*positionRight == lengthRight) {
					right = NULL;
					sfxChannels[sfxChannel + 1].sfxID = SFX_NONE;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// the mixer callbacks
// ---------------------------------------------------------------------------

void audioSFXCallback0(void *buf, unsigned int length, void *userdata) {
	audioSFXCallbackMixer(buf, length, 0);
}
void audioSFXCallback1(void *buf, unsigned int length, void *userdata) {
	audioSFXCallbackMixer(buf, length, 2);
}
void audioSFXCallback2(void *buf, unsigned int length, void *userdata) {
	audioSFXCallbackMixer(buf, length, 4);
}
void audioSFXCallback3(void *buf, unsigned int length, void *userdata) {
	audioSFXCallbackMixer(buf, length, 6);
}

// ---------------------------------------------------------------------------
// init the audio subsystem
// ---------------------------------------------------------------------------

void audioInit(void) {

	int i;

	pspAudioInit();

	// reset the SFX channels
	for (i = 0; i < 4*2; i++)
		sfxChannels[i].sfxID = SFX_NONE;

	// SFX will take channels 1-4 (two mono samples per channel == 8 concurrent samples can be played)
	pspAudioSetChannelCallback(1, audioSFXCallback0, 0);
	pspAudioSetChannelCallback(2, audioSFXCallback1, 0);
	pspAudioSetChannelCallback(3, audioSFXCallback2, 0);
	pspAudioSetChannelCallback(4, audioSFXCallback3, 0);
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

	sfxs = sfxList;
	sfxsN = sfxN;
}

// ---------------------------------------------------------------------------
// set the status of the BGM
// ---------------------------------------------------------------------------

void audioSetBGM(int status) {

	audioBGM = status;

	if (status == YES)
		audioPlayBGM(audioBGMLast);
	else
		audioPlayBGM(-1);
}

// ---------------------------------------------------------------------------
// set the status of SFX
// ---------------------------------------------------------------------------

void audioSetSFX(int status) {

	audioSFX = status;
}

// ---------------------------------------------------------------------------
// play the supplied BGM
// ---------------------------------------------------------------------------

void audioPlayBGM(int bgm) {

	if (audioBGMLoaded == YES)
		MP3_End();

	audioBGMLoaded = NO;

	if (bgm < 0)
		return;

	audioBGMLast = bgm;

	// mp3 playing will take channel 0
	MP3_Init(0);

	if (MP3_Load(bgms[bgm]) == 0) {
		audioBGMLoaded = NO;
		MP3_End();
	}
	else {
		audioBGMLoaded = YES;
		MP3_Play();
	}
}

// ---------------------------------------------------------------------------
// play the supplied SFX
// ---------------------------------------------------------------------------

void audioPlaySFX(int sfx) {

	int i;

	if (audioSFX == NO)
		return;

	// find a free channel
	for (i = 0; i < 4*2; i++) {
		if (sfxChannels[i].sfxID == SFX_NONE) {
			// reserve the channel
			sfxChannels[i].position = 0;
			sfxChannels[i].sfxID = sfx;
			return;
		}
	}
}
