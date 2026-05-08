/*
 *   Linear laser class
 *
 */

#pragma once

#include "LASER.H"

class C_LLASER;
class C_PLAYRANK;
class C_RNG;
class C_VIV;

class C_LASER {
public:
	// 標準レーザーコマンド構造体
	LASER_CMD LaserCmd;

private:
	C_LLASER const& LLaser;
	C_PLAYRANK const& PlayRank;
	C_RNG& RNG;
	C_VIV& Viv;

	// レーザーの本数
	uint16_t LaserNow;

	// レーザー格納用構造体
	std::array<LASER_DATA, LASER_MAX> Laser;

	// レーザー順番維持用配列
	std::array<uint16_t, LASER_MAX> LaserInd;

	// レーザーの進行方向をセットする
	uint8_t laser_dir(uint16_t i);

	// 種類により分岐し座標を更新する
	void Lmove(LASER_DATA *lp);

	// レーザーの当たり判定
	void laser_hitchk(LASER_DATA *lp);

	// 反射レーザーの移動
	void REFL_move(LASER_DATA *lp);

	// ﾘﾌﾚｸﾀｰとの当たり判定(ﾋｯﾄ->非０)
	int REFL_hit(const LASER_DATA *lp);

public:
	C_LASER(
		C_LLASER const& LLaser,
		C_PLAYRANK const& PlayRank,
		C_RNG& RNG,
		C_VIV& Viv
	) noexcept : LLaser(LLaser), PlayRank(PlayRank), RNG(RNG), Viv(Viv) {
	}

	// レーザーをセットする(難易度変更"有り")
	void Set(void);

	// レーザーをセットする(難易度変更"無し")
	void SetEX(void);

	// レーザーを動かす
	void Move(void);

	// レーザー全てに消去エフェクトをかける
	void Clear(void);

	// レーザー順序用配列の初期化
	void IndSet(void);

	const LASER_DATA_CSPAN Data(void) const noexcept {
		return Laser;
	}

	const std::span<const uint16_t> Inds(void) const {
		return std::span(LaserInd).first(LaserNow);
	}
};
