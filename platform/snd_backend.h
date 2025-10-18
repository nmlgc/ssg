/*
 *   Platform-specific PCM sound interface
 *
 */

#pragma once

#include "game/snd.h"
struct SDL_AudioSpec;

bool SndBackend_Init(void);
void SndBackend_Cleanup(void);

// The platform-independent layer always calls this after SndBackend_Init().
bool SndBackend_BGMInit(void);

// The platform-independent layer always calls this before SndBackend_Cleanup().
void SndBackend_BGMCleanup(void);

void SndBackend_BGMUpdateVolume(void);

namespace BGM {
	struct TRACK;
}
bool SndBackend_BGMLoad(std::shared_ptr<BGM::TRACK> track);
void SndBackend_BGMPlay(void);
void SndBackend_BGMStop(void);

// Returns the amount of milliseconds that the subsystem has been playing the
// BGM track for.
std::chrono::milliseconds SndBackend_BGMPlayTime(void);

void SndBackend_BGMUpdateVolume(void);
void SndBackend_BGMUpdateTempo(void);

// The platform-independent layer always calls this after SndBackend_Init().
bool SndBackend_SEInit(void);

// The platform-independent layer always calls this before SndBackend_Cleanup().
void SndBackend_SECleanup(void);

void SndBackend_SEUpdateVolume(void);

bool SndBackend_SELoad(
	uint8_t id,
	SND_INSTANCE_ID max,
	const SDL_AudioSpec& spec,
	BYTE_BUFFER_BORROWED pcm
);
void SndBackend_SEPlay(uint8_t id, int x = SND_X_MID, bool loop = false);
void SndBackend_SEStop(uint8_t id);

// Pause or resume all playing sounds if the window loses focus
void SndBackend_PauseAll();
void SndBackend_ResumeAll();
