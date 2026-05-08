/*
 *   3D effect classes
 *
 */

#pragma once

#include "EFFECT3D.H"

class C_RNG;

class C_EFFECT3D {
private:
	C_RNG& RNG;

	EFFECT3D_TYPE Active;
	uint16_t cube_d;
	uint16_t cube_dx;
	uint16_t cube_dy;

	Cube3D Cube[CUBE_MAX];
	Star2D Star[STAR_MAX];
	Rock3D Rock[ROCK_MAX];
	WFLine2D WFLine;
	FakeECLString FakeECLStr[FAKE_ECLSTR_MAX];
	Stg6Raster S6Ras[S6RASTER_MAX];
	Stg6Star S6Star[S3STAR_MAX]; // 兼用モノなのだ

public:
	void Init3DCube(void);
	void Move3DCube(void);

	void InitFakeECL(void);
	void MoveFakeECL(void);

	// ステージ４の背景となる岩の集団
	void InitStg4Rock(void);

	void MoveStg4Rock(void);

	// ６面ラスター初期化
	void InitStg6Raster();

	// ６面ラスター動作
	void MoveStg6Raster();

	// ３面高速星初期化
	void InitStg3Star();

	// ３面高速星動作
	void MoveStg3Star();

public:
	C_EFFECT3D(C_RNG& RNG) noexcept : RNG(RNG) {
	}

	// コマンド送信
	void SendCmdStg4Rock(uint8_t Cmd, uint8_t Param);

	// Initializes and activates the given effect.
	void Set(EFFECT3D_TYPE effect);

	// Updates the given effect.
	void Move(void);

	auto GetActive(void) const {
		return Active;
	}

	CUBE3D_CSPAN DataCube(void) const {
		return Cube;
	}
	STAR2D_CSPAN DataStar(void) const {
		return Star;
	}
	ROCK3D_CSPAN DataRock(void) const {
		return Rock;
	}
	const WFLine2D& DataWFLine(void) const {
		return WFLine;
	}
	FAKE_ECLSTR_CSPAN DataFakeECL(void) const {
		return FakeECLStr;
	}
	STG6RASTER_CSPAN DataS6Ras(void) const {
		return S6Ras;
	}
	STG6STAR_CSPAN DataS6Star(void) const {
		return S6Star;
	}
};
