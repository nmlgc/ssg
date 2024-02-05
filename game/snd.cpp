/*
 *   Sound interface
 *
 */

#include "game/snd.h"
#include "platform/snd_backend.h"

bool SndInit(void)
{
	return SndBackend_Init();
}

void SndCleanup(void)
{
	return SndBackend_Cleanup();
}

bool Snd_SELoad(BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max)
{
	return SndBackend_SELoad(std::move(buffer), id, max);
}

void Snd_SEPlay(uint8_t id, int x, bool loop)
{
	return SndBackend_SEPlay(id, x, loop);
}

void Snd_SEStop(uint8_t id)
{
	return SndBackend_SEStop(id);
}

void Snd_SEStopAll(void)
{
	for(auto i = 0; i < SND_OBJ_MAX; i++) {
		SndBackend_SEStop(i);
	}
}
