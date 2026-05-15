/*
 *   Logic part of the SCL parser
 *
 */

#include "ssg/internal/Stage.hpp"
#include "ssg/Gian.h"
#include "ssg/Hook.h"
#include "ssg/Round.h"
#include "ssg/SCL.h"
#include "ssg/internal/Boss.hpp"
#include "ssg/internal/Effect3D.hpp"
#include "ssg/internal/PRankCtrl.hpp"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/logic/endian.h"

bool C_STAGE::Set(BUFFER_OWNED&& data, uint8_t stage)
{
	SCL_Head = std::move(data);
	SCL_Now = SCL_Head.get();
	GameCount = 0;
	GameStage = stage;
	SclInfo.MsgFlag = false;
	SclInfo.ReturnFlag = false;
	Effect3D.Set(EFFECT3D_TYPE::NONE);

	return (SCL_Head.get() != nullptr);
}

// p:SCL_ENEMY以降の敵配置データ //
void C_STAGE::Move(INPUT_BITS input)
{
	bool bFlag = true;
	bool CtrlFlag = false;

	while(bFlag) {
		bool pause = false;
		bool error = false;
		const auto *cmd = SCL_Now;
		switch(cmd[0]) {
		case(SCL_KEY): // キー入力待ち
			break;

		case(SCL_TIME): {
			const auto temp = U32LEAt(&cmd[1]);
			// メッセージウィンドウがオープンしている場合 //
			if(SclInfo.MsgFlag) {
				if(!Round.MsgEnabled || (input & KEY_SKIP)) {
					CtrlFlag = true;
				}

				if(CtrlFlag) {
					GameCount += (temp - GameCount) / 3;
				} else if(input & KEY_RETURN) {
					if(!SclInfo.ReturnFlag) {
						GameCount = temp;
						SclInfo.ReturnFlag = true;
					}
				} else {
					SclInfo.ReturnFlag = false;
				}
			}

			if(temp > GameCount) {
				bFlag = false;
			} else {
				SCL_Now += 5; // cmd(1)+time(4)
			}
		} break;

		case(SCL_ENEMY):
			// ボス出現中は出て来ちゃダメ
			if(Boss.NumAlive() == 0) {
				if(auto *e = Enemy.FindFree()) {
					const int16_t x = I16LEAt(&cmd[1 + 0]);
					const int16_t y = I16LEAt(&cmd[1 + 2]);
					const uint8_t id = cmd[1 + 4];
					Enemy.InitEnemyDataSTD(e, x, y, id);
				}
			}
			SCL_Now += 6; // cmd(1)+x(2)+y(2)+id(1)
			break;

		case(SCL_BOSS): { // ボスをセットする(X(16),Y(16),ID(8))
			const auto x = I16LEAt(&cmd[1 + 0]); // ボス初期Ｘ
			const auto y = I16LEAt(&cmd[1 + 2]); // ボス初期Ｙ
			const auto id = cmd[1 + 2 + 2];      // ボスＩＤ
			Boss.Set(x, y, id);
			SCL_Now += (1 + 2 + 2 + 1); // cmd+x+y+id
		} break;

		case(SCL_BOSSDEAD): // ボスを強制的に破壊する(Level2 命令Only)
			Boss.KillAll();
			SCL_Now++;
			break;

		case(SCL_MWOPEN): // メッセージウィンドウを開く
			SclInfo.MsgFlag = true;
			SCL_Now++;
			break;

		case(SCL_MWCLOSE): // メッセージウィンドウを閉じる
			SclInfo.MsgFlag = false;
			SCL_Now++;
			break;

		case(SCL_MSG): // メッセージを出力する
			SCL_Now += (strlen(std::bit_cast<const char *>(cmd + 1)) + 2);
			break;

		case(SCL_FACE): // 顔を表示する
			SCL_Now += 2;
			break;

		case(SCL_LOADFACE): // 顔グラをロードする(SurfaceID,FileNo)
			SCL_Now += 3;
			break;

		case(SCL_NPG): // 新しいページに変更する
			SCL_Now++;
			break;

		case(SCL_SSP): // スクロールスピード変更
			SCL_Now += 3;
			break;

		case(SCL_MUSIC):
			SCL_Now += 2;
			break;

		case(SCL_DELENEMY):
			Enemy.IndSet();
			SCL_Now++;
			break;

		case(SCL_EFC):
			switch(cmd[1]) {
			case(SEFC_STG6CUBE): // ６面キューブ
				Effect3D.Set(EFFECT3D_TYPE::S6_CUBE);
				break;
			case(SEFC_STG6RNDECL): // ６面ＥＣＬ羅列
				Effect3D.Set(EFFECT3D_TYPE::S6_FAKE_ECL);
				break;
			case(SEFC_STG4ROCK): // ４面岩
				Effect3D.Set(EFFECT3D_TYPE::S4_ROCK);
				break;
			case(SEFC_STG4LEAVE): // ４面岩画面外へ
				if(Effect3D.GetActive() != EFFECT3D_TYPE::S4_ROCK) {
					break;
				}
				Effect3D.SendCmdStg4Rock(STG4ROCK_LEAVE, 0);
				break;
			case(SEFC_STG6RASTER): // ６面ラスター
				Effect3D.Set(EFFECT3D_TYPE::S6_RASTER);
				break;
			}
			SCL_Now += 2;
			break;

		case(SCL_WAITEX): { // 特殊待ち <cmd1>,<opt4>
			switch(cmd[1]) {
			case(SWAIT_BOSSHP): // 残りＨＰ
				pause = (Boss.GetHPSum() > U32LEAt(&cmd[2]));
				break;
			case(SWAIT_BOSSLEFT): // 残りボス数
				pause = (Boss.NumAlive() > U32LEAt(&cmd[2]));
				break;
			}
			if(!pause) {
				SCL_Now += (1 + 1 + 4);
			}
		} break;

		case(SCL_MAPPALETTE):
		case(SCL_ENEMYPALETTE):
			SCL_Now++;
			break;

		case(SCL_STAGECLEAR):
		case(SCL_GAMECLEAR):
		case(SCL_EXTRACLEAR):
		case(SCL_END): // カウントも変更させずにリターンする
			pause = true;
			break;

		default: // 未実装 or ばぐ
			error = true;
			pause = true;
			return;
		}
		Hooks.SCL_Op(cmd, error, &Hooks);
		if(pause) {
			return;
		}
	}

	GameCount++;

	if((GameCount & 0x3f) == 0) {
		PlayRank.Add((GameStage == STAGE_EXTRA) ? 1 : (1 + (GameStage / 3)));
	}
}
