/*                                                                           */
/*   DS_UTY.h   効果音関連の処理                                             */
/*                                                                           */
/*                                                                           */

#pragma once


// ヘッダファイル //
#include "platform/buffer.h"
#include <stdint.h>

// 定数＆マクロ //
#define SND_OBJ_MAX			30				// 効果音の種類の最大数
#define SND_XMID_DEFAULT	(320*64)		// Ｘ座標の中心のデフォルト値


// 初期化など //
bool SndInit(int x_mid = SND_XMID_DEFAULT);
void SndCleanup(void);

bool SndWAVLoad(BYTE_BUFFER_OWNED buffer, uint8_t id, int max);

// 再生＆停止 //
void SndPlay(uint8_t id);
void SndPlayEX(uint8_t id, int x, bool loop);
void SndStop(uint8_t id);
void SndStopAll(void);
