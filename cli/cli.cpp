/*
 *   秋霜玉 / Shuusou Gyoku replay CLI
 *
 */

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>

#include "hatoyama/app/file.h"
#include "hatoyama/app/replay_cli.hpp"
#include <ssg/API.h>
#include <ssg/Gian.h>
#include <ssg/Replay.h>
#include <ssg/SCL.h>
#include <ssg/internal/LZ.hpp>
#include <ssg/internal/SSG.hpp>

REPLAY_CLI_RET Simulate(
	const ARGS_GIVEN& args, std::u8string_view fn_view, const PACKFILE_READ& dat
)
{
	const char8_t *fn = fn_view.data();
	assert(fn[fn_view.size()] == '\0');

	const auto t_load = SDL_GetTicksNS();
	const auto stage = Replay::OldStageNumDetect(fn_view);
	if(!stage) {
		// People are very likely to pass `*.DAT` in conjunction with `-q`,
		// which will catch packfiles, score files, and the original
		// configuration, producing a lot of noise if we print an error here...
		if(args.quiet) {
			return REPLAY_CLI_RET::OK;
		}
		return SDL_errorf("`%s`: failed to detect stage\n", fn);
	}
	const auto fil = SDL_LoadFile(fn);
	if(!fil) {
		// This error already includes the file name.
		return SDL_errorf("%s\n", SDL_GetError());
	}
	auto replay = Replay::OldLoad(BUFFER_BORROWED{ fil });
	if(!replay) {
		return SDL_errorf("`%s`: invalid one-stage replay\n", fn);
	}
	const auto& info = *replay.Info;

	auto *ssg = SSGNew();
	if(!ssg) {
		return SDL_fatalf("`%s`: out of memory\n", fn);
	}
	defer(ssg = SSG_Free(ssg));

	ssg->RoundInitFromReplayOld(*replay.Info, stage);
	if(!ssg->StageLoadFromDAT(dat, stage)) {
		return SDL_errorf("`%s`: failed to load stage data\n", fn);
	}

	// Track gameplay events
	// ---------------------

	struct SSG_CONTEXT {
		decltype(info.FrameCount) frame = 0;
		unsigned int misses = 0;
		unsigned int bombs = 0;
		bool cleared = false;
	};
	SSG_CONTEXT context;

	ssg->Hooks.Context = &context;
	ssg->Hooks.Stage_Clear = [](HOOKS *hooks) {
		static_cast<SSG_CONTEXT *>(hooks->Context)->cleared = true;
	};
	ssg->Hooks.Viv_Dead = [](HOOKS *hooks) {
		static_cast<SSG_CONTEXT *>(hooks->Context)->misses++;
	};
	ssg->Hooks.Viv_Bomb = [](HOOKS *hooks) {
		static_cast<SSG_CONTEXT *>(hooks->Context)->bombs++;
	};
	// ---------------------

	const auto t_sim = SDL_GetTicksNS();
	while(context.frame < info.FrameCount) {
		ssg->Move(replay.Frames[context.frame]);
		context.frame++;
		if(ssg->Viv.GameOverTimer) {
			break;
		}
	}

	// Skip over the additional quit frame that the original game inserts on a
	// Game Over
	if(
		(context.frame < info.FrameCount) &&
		(replay.Frames[context.frame] & KEY_ESC)
	) {
		context.frame++;
	}
	const int ret = (context.frame < info.FrameCount);

#define W "9"
	const auto t_done = SDL_GetTicksNS();
	const auto d_load = (t_sim - t_load);
	const auto d_sim = (t_done - t_sim);
	const auto d_load_ms = (d_load / 1000000.0);
	const auto d_sim_ms = (d_sim / 1000000.0);
	const auto d_fps = (context.frame / (d_sim_ms / 1000.0));
	if(ret) {
		SDL_LogMessage(
			SDL_LOG_CATEGORY_APPLICATION,
			SDL_LOG_PRIORITY_WARN,
			"`%s`: simulation stopped after %u/%u frames (%2.1f%%)",
			std::bit_cast<const char *>(fn),
			context.frame,
			info.FrameCount,
			((100.0 * context.frame) / info.FrameCount)
		);
	} else {
		if(args.quiet) {
			return REPLAY_CLI_RET::OK;
		}
		const auto *cleared_str = (context.cleared ? "Yes" : "No");
		SDL_printf("%s:\n", fn);
		SDL_printf("   Stage cleared: %" W "s\n", cleared_str);
		SDL_printf("          Frames: %" W "u\n", context.frame);
	}
	SDL_printf("           Score: %" W "lld\n", ssg->Viv.score);
	SDL_printf("          Misses: %" W "u\n", context.misses);
	SDL_printf("           Bombs: %" W "u\n", context.bombs);
	SDL_printf("            Seed:  %08x\n", replay.Info->RndSeed);
	SDL_printf("       Load time: %" W ".1f ms\n", d_load_ms);
	SDL_printf(" Simulation time: %" W ".1f ms\n", d_sim_ms);
	SDL_printf("Simulation speed: %" W ".0f FPS\n", d_fps);
	SDL_printf("-------------------------------\n");
	return (ret ? REPLAY_CLI_RET::WARN : REPLAY_CLI_RET::OK);
}

int main(int argc, char **argv)
{
	return std::to_underlying(ReplayCLI(
		argc, argv, std::bit_cast<const char*>(LogicPackfileBasename())
	));
}

REPLAY_CLI_RET ReplayCLI_Run(ARGS_GIVEN& args)
{
	auto *dat_io = SDL_IOFromFile(args.dat_fn, "rb");
	if(!dat_io) {
		// This error already includes the file name.
		return SDL_fatalf("%s\n", SDL_GetError());
	}
	const auto dat_raw = SDL_LoadFile_IO(dat_io, true);
	if(!dat_raw) {
		return SDL_fatalf(
			"failed to load %s: %s\n", args.dat_fn, SDL_GetError()
		);
	}
	const auto dat = FilStartR(BUFFER_BORROWED{ dat_raw });
	if(!dat) {
		return SDL_fatalf("failed to verify %s\n", args.dat_fn);
	}

	auto ret = REPLAY_CLI_RET::OK;
	for(const auto *arg : args.positionals) {
		ret |= ReplayCLI_ProcessPositional(arg, [&](auto fn) {
			return Simulate(args, fn, dat);
		});
		if(!!(ret & REPLAY_CLI_RET::FATAL)) {
			return ret;
		}
	}
	return ret;
}
