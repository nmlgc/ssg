/*
 *   Platform-specific PCM sound interface
 *
 */

#pragma once

#include "game/snd.h"

bool SndBackend_Init(void);
void SndBackend_Cleanup(void);

// The platform-independent layer always calls this after SndBackend_Init().
bool SndBackend_BGMInit(void);

// The platform-independent layer always calls this before SndBackend_Cleanup().
void SndBackend_BGMCleanup(void);

// The platform-independent layer always calls this after SndBackend_Init().
bool SndBackend_SEInit(void);

// The platform-independent layer always calls this before SndBackend_Cleanup().
void SndBackend_SECleanup(void);

bool SndBackend_SELoad(
	BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max
);
void SndBackend_SEPlay(uint8_t id, int x = SND_X_MID, bool loop = false);
void SndBackend_SEStop(uint8_t id);

// Pause or resume all playing sounds if the window loses focus
void SndBackend_PauseAll();
void SndBackend_ResumeAll();
