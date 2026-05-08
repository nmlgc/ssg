/*
 *   SCL parser
 *
 */

#pragma once

#include "ssg/Input.h"
#include "hatoyama/logic/buffer.h"

class C_BOSS;
class C_EFFECT3D;
class C_ENEMY;
class C_PLAYRANK;
struct HOOKS;
struct ROUND_PARAMS;


// ＳＣＬ管理用構造体 //
struct SCL_INFO {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
};

class C_STAGE {
private:
	HOOKS& Hooks;
	const ROUND_PARAMS& Round;
	C_BOSS& Boss;
	C_EFFECT3D& Effect3D;
	C_ENEMY& Enemy;
	C_PLAYRANK& PlayRank;

	BUFFER_OWNED SCL_Head;
	const uint8_t *SCL_Now;
	uint32_t GameCount;

	uint8_t GameStage;

	// ＳＣＬに関する情報
	SCL_INFO SclInfo;

public:
	C_STAGE(
		HOOKS& Hooks,
		const ROUND_PARAMS& Round,
		C_BOSS& Boss,
		C_EFFECT3D& Effect3D,
		C_ENEMY& Enemy,
		C_PLAYRANK& PlayRank
	) noexcept :
		Hooks(Hooks),
		Round(Round),
		Boss(Boss),
		Effect3D(Effect3D),
		Enemy(Enemy),
		PlayRank(PlayRank) {
	}

	// Returns `true` if [data] is a valid pointer.
	bool Set(BUFFER_OWNED&& data, uint8_t stage);

	void Move(INPUT_BITS input);

	auto Number(void) const {
		return GameStage;
	}

	auto SCLCount(void) const {
		return GameCount;
	}

	bool InMsg(void) const {
		return SclInfo.MsgFlag;
	}
};
