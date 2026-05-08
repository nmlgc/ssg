/*
 *   Stage enemy class
 *
 */

#pragma once

#include "ENEMY.H"
#include "hatoyama/logic/buffer.h"

class C_BOSS;
class C_EFFECT3D;
class C_HLASER;
class C_ITEM;
class C_LASER;
class C_LLASER;
class C_PLAYRANK;
class C_TAMA;
class C_VIV;
struct C_RNG;
struct HOOKS;

class C_ENEMY {
private:
	HOOKS& Hooks;
	C_BOSS& Boss;
	C_EFFECT3D& Effect3D;
	C_HLASER& HLaser;
	C_ITEM& Item;
	C_LASER& Laser;
	C_LLASER& LLaser;
	C_PLAYRANK const& PlayRank;
	C_RNG& RNG;
	C_TAMA& Tama;
	C_VIV& Viv;

	//// 敵変数 ////
	uint16_t EnemyNow;

	// 特殊角度の現在値
	uint8_t EnemyEXDEG;

	// 特殊角度の増分
	uint8_t EnemyEXDEG_D;

	BUFFER_OWNED ECL_Head;
	std::array<ENEMY_DATA, ENEMY_MAX> Enemy;
	std::array<uint16_t, ENEMY_MAX> EnemyInd;
	ANIME_DATA Anime[ANIME_MAX];

	bool DamageApply(ENEMY_DATA& e, int damage);

public:
	C_ENEMY(
		HOOKS& Hooks,
		C_BOSS& Boss,
		C_EFFECT3D& Effect3D,
		C_HLASER& HLaser,
		C_ITEM& Item,
		C_LASER& Laser,
		C_LLASER& LLaser,
		C_PLAYRANK const& PlayRank,
		C_RNG& RNG,
		C_TAMA& Tama,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Boss(Boss),
		Effect3D(Effect3D),
		HLaser(HLaser),
		Item(Item),
		Laser(Laser),
		LLaser(LLaser),
		PlayRank(PlayRank),
		RNG(RNG),
		Tama(Tama),
		Viv(Viv) {
	}

	//// 敵制御関数 ////
	ENEMY_DATA *FindFree(void);

	// Returns `true` if [ecl] is a valid pointer.
	bool Set(BUFFER_OWNED&& ecl, uint8_t stage);

	// 敵データを初期化する(x,y は x64 で指定のこと) //
	void InitEnemyDataX64(ENEMY_DATA *e, int x, int y, uint32_t id);

	// 敵データを初期化する(x,y は非x64(ランダム可能) で指定のこと) //
	void InitEnemyDataSTD(ENEMY_DATA *e, short x, short y, uint32_t id);

	// 敵を動かす
	void Move(void);

	// 敵の順序設定用配列の初期化をする
	void IndSet(void);

	// 雑魚を消滅させる
	void Clear(void);

	// 敵にダメージを与える
	bool Damage(int x, int y, int damage);

	// ｙ上方向無限Ver.敵にダメージを与える
	bool Damage2(int x, int y, int damage);

	// ナナメレーザーの当たり判定
	void Damage3(int x, int y, uint8_t d);

	// すべての敵にダメージを与える
	void Damage4(int damage);

	void AnimeMove(ENEMY_DATA *e) const;
	void Delete(ENEMY_DATA& e);
	void Explode(ENEMY_DATA& e);


	// 敵をＥＣＬに従って動かす
	void ECL_Parse(ENEMY_DATA *e);

	// 強制的に ECL ブロック間を移動する //
	void ECL_LongJump(ENEMY_DATA *e, uint32_t EclID) const;

	decltype(EnemyNow) NumAlive(void) const {
		return EnemyNow;
	}

	const ENEMY_DATA_CSPAN Data(void) const {
		return Enemy;
	}

	const std::span<const uint16_t> Inds(void) const {
		return std::span(EnemyInd).first(EnemyNow);
	}

	const ANIME_DATA_CSPAN AnimeData(void) const noexcept {
		return Anime;
	}
};
