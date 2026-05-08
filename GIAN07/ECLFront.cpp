/*
 *   Frontend ECL handler
 *
 */

#ifdef SCRIPT_TRACE
#include <printf/printf.h>
#endif

#include "ECLFront.h"
#include "BossHPG.h"
#include "EFFECT.H"
#include "LogicInstance.h"
#include "SCROLL.H"
#include "hatoyama/engine/debug.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/endian.h"


// ＥＣＬデバッグ用マクロ //
static void ECL_DEBUG(const char *s, auto param)
{
#ifdef SCRIPT_TRACE
	char8_t _ECL_Debug[1000];
	const auto size = sprintf(std::bit_cast<char *>(&_ECL_Debug[0]), s, param);
	if(size <= 0) {
		return;
	}
	DebugLog({ _ECL_Debug, static_cast<size_t>(size) });
#endif
}

void ECL_Frontend(
	const uint8_t *cmd, const ENEMY_DATA *e, bool8_t error, HOOKS *
)
{
	switch(*cmd) {
	case(ECL_CEFC): {
		const auto x = (e->x + PixelToWorld(I16LEAt(&cmd[1 + 0])));
		const auto y = (e->y + PixelToWorld(I16LEAt(&cmd[1 + 2])));
		CEffectSet(x, y, cmd[1 + 2 + 2]);
	} break;

	case(ECL_STG3EFC):
		ScrollCommand(SCMD_STG3STAR);
		break;

	case(ECL_BOSSSET):
		BossHPG_Update(SSG.Boss.GetHPSum());
		break;

	case(ECL_SETUP): ECL_DEBUG("ECL_SETUP", 0); break;
	case(ECL_END):   ECL_DEBUG("ECL_END", 0); break;
	case(ECL_JMP):   ECL_DEBUG("ECL_JMP", 0); break;
	case(ECL_LOOP):  ECL_DEBUG("ECL_LOOP : %d", e->rep_c); break;
	case(ECL_CALL):  ECL_DEBUG("ECL_CALL", 0); break;
	case(ECL_RET):   ECL_DEBUG("ECL_RET", 0); break;
	case(ECL_JHPL):  ECL_DEBUG("ECL_JHPL : %u", U32LEAt(&cmd[1])); break;
	case(ECL_JHPS):  ECL_DEBUG("ECL_JHPS : %u", U32LEAt(&cmd[1])); break;
	case(ECL_JDIF):  ECL_DEBUG("ECL_JDIF", 0); break;
	case(ECL_JDSB):  ECL_DEBUG("ECL_JDSB", 0); break;
	case(ECL_JFCL):  ECL_DEBUG("ECL_JFCL", 0); break;
	case(ECL_JFCS):  ECL_DEBUG("ECL_JFCS", 0); break;
	case(ECL_STI):
		if(error) {
			ECL_DEBUG("不正な割り込みベクタ %d へのアクセス", cmd[1 + 4]);
		}
		break;

	case(ECL_NOP):   ECL_DEBUG("ECL_NOP : %d", e->cmd_c); break;
	case(ECL_NOPSC): ECL_DEBUG("ECL_NOPSC : %d", e->cmd_c); break;
	case(ECL_ACC):   ECL_DEBUG("ECL_ACC : %d", e->cmd_c); break;
	case(ECL_MOV):   ECL_DEBUG("ECL_MOV : %d", e->cmd_c); break;
	case(ECL_ROL):   ECL_DEBUG("ECL_ROL : %d", e->cmd_c); break;
	case(ECL_LROL):  ECL_DEBUG("ECL_LROL : %d", e->cmd_c); break;
	case(ECL_WAVX):  ECL_DEBUG("ECL_WAVX : %d", e->cmd_c); break;
	case(ECL_WAVY):  ECL_DEBUG("ECL_WAVY : %d", e->cmd_c); break;
	case(ECL_MXA):   ECL_DEBUG("ECL_MXA : %d", e->cmd_c); break;
	case(ECL_MYA):   ECL_DEBUG("ECL_MYA : %d", e->cmd_c); break;
	case(ECL_MXYA):  ECL_DEBUG("ECL_MXYA : %d", e->cmd_c); break;
	case(ECL_MXS):   ECL_DEBUG("ECL_MXS : %d", e->cmd_c); break;
	case(ECL_MYS):   ECL_DEBUG("ECL_MYS : %d", e->cmd_c); break;
	case(ECL_MXYS):  ECL_DEBUG("ECL_MXYS : %d", e->cmd_c); break;
	case(ECL_DEGA):  ECL_DEBUG("ECL_DEGA : %u", cmd[1]); break;

	case(ECL_DEGR):
		ECL_DEBUG("ECL_DEGR : %d", Cast::sign<int8_t>(cmd[1]));
		break;

	case(ECL_DEGX):       ECL_DEBUG("ECL_DEGX", 0); break;
	case(ECL_DEGS):       ECL_DEBUG("ECL_DEGS", 0); break;
	case(ECL_SPDA):       ECL_DEBUG("ECL_SPDA", 0); break;
	case(ECL_SPDR):       ECL_DEBUG("ECL_SPDR", 0); break;
	case(ECL_XYA):        ECL_DEBUG("ECL_XYA", 0); break;
	case(ECL_XYR):        ECL_DEBUG("ECL_XYR", 0); break;
	case(ECL_DRAW_ON):    ECL_DEBUG("ECL_DRAW_ON", 0); break;
	case(ECL_DRAW_OFF):   ECL_DEBUG("ECL_DRAW_OFF", 0); break;
	case(ECL_CLIP_ON):    ECL_DEBUG("ECL_CLIP_ON", 0); break;
	case(ECL_CLIP_OFF):   ECL_DEBUG("ECL_CLIP_OFF", 0); break;
	case(ECL_DAMAGE_ON):  ECL_DEBUG("ECL_DAMAGE_ON", 0); break;
	case(ECL_DAMAGE_OFF): ECL_DEBUG("ECL_DAMAGE_OFF", 0); break;
	case(ECL_HITSB_ON):   ECL_DEBUG("ECL_HITSB_ON", 0); break;
	case(ECL_HITSB_OFF):  ECL_DEBUG("ECL_HITSB_OFF", 0); break;
	case(ECL_RLCHG_ON):   ECL_DEBUG("ECL_RLCHG_ON", 0); break;
	case(ECL_RLCHG_OFF):  ECL_DEBUG("ECL_RLCHG_OFF", 0); break;
	case(ECL_PSE):        ECL_DEBUG("ECL_PSE", 0); break;

	case(ECL_MOVR):
		if(error) {
			DebugOut(u8"ナゾのレジスタ指定++");
		}
		break;

	case(ECL_MOVC):
	case(ECL_INC):
	case(ECL_DEC):
	case(ECL_ADD):
	case(ECL_SUB):
	case(ECL_MOD):
	case(ECL_RND):
		if(error) {
			DebugOut(u8"ナゾのレジスタ指定");
		}
		break;

	default:
		if(error) {
			ECL_DEBUG("Unrecognizable Operation Code %02x", cmd[0]);
		}
		break;
	}
}
