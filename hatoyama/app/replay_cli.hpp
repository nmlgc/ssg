/*
 *   Common code for replay validation CLIs
 *
 */

#pragma once

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR // A "Region Flag" in `wingdi.h`. Seriously.
#else
#include <unistd.h>
#endif

#include "api/api.h"
#include "app/path.h"
#include "logic/defer.h"
#include "logic/enum_flags.h"
#include "obj/version.h"
#include <assert.h>

struct ARGS_GIVEN {
	const char8_t *dat_fn = nullptr;
	std::span<char *> positionals; // Actually `char8_t *`.
	bool quiet = false;
};

enum class REPLAY_CLI_RET : uint8_t {
	_HAS_BITFLAG_OPERATORS,

	OK = 0,
	FATAL = (1 << 0), // CLI can't continue running
	ERROR = (1 << 1), // ≥1 replay failed to start running
	WARN = (1 << 2),  // ≥1 replay failed to complete
};

/// Implemented by the specific game
/// --------------------------------

REPLAY_CLI_RET ReplayCLI_Run(ARGS_GIVEN& args);
/// --------------------------------

void SDL_logf(SDL_LogPriority priority, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, priority, fmt, va);
	va_end(va);
}

void SDL_printf(const char* fmt, auto&&... args)
{
	SDL_logf(SDL_LOG_PRIORITY_INFO, fmt, args...);
}

REPLAY_CLI_RET SDL_fatalf(const char* fmt, auto&&... args)
{
	SDL_logf(SDL_LOG_PRIORITY_CRITICAL, fmt, args...);
	return REPLAY_CLI_RET::FATAL;
}

REPLAY_CLI_RET SDL_errorf(const char* fmt, auto&&... args)
{
	SDL_logf(SDL_LOG_PRIORITY_ERROR, fmt, args...);
	return REPLAY_CLI_RET::ERROR;
}

REPLAY_CLI_RET SDL_warnf(const char* fmt, auto&&... args)
{
	SDL_logf(SDL_LOG_PRIORITY_WARN, fmt, args...);
	return REPLAY_CLI_RET::WARN;
}

enum class OPTION_ID : uint16_t {
	ERROR = 0,
	LAST, // First positional argument
	END,  // Positional arguments start after this one
	DAT,
	HELP,
	QUIET,
	VERSION,
};

struct OPTION {
	OPTION_ID id = OPTION_ID::LAST;
	char switch_short = '\0';
	std::string_view switch_long;
	std::string_view help;
	std::string_view param_desc = "";
};

static constinit auto OPTS = ([] {
	std::array<OPTION, 4> ret = {{
		{ OPTION_ID::DAT, 'd', "dat", "Custom path to the game data archive" },
		{ OPTION_ID::HELP, 'h', "help", "Print this usage text" },
		{
			OPTION_ID::QUIET,
			'q',
			"quiet",
			"Only report replays that don't simulate completely",
		},
		{ OPTION_ID::VERSION, 'v', "version", "Print version info" }
	}};
	std::ranges::sort(ret, [](const OPTION& a, const OPTION& b) {
		if(a.switch_short == b.switch_short) {
			throw "Duplicated short switch";
		}
		if(a.switch_long == b.switch_long) {
			throw "Duplicated long switch";
		}
		return (a.switch_short < b.switch_short);
	});
	return ret;
})();

OPTION_ID OptionFind(std::span<const OPTION> opts, const char *arg)
{
	if(arg[0] != '-') {
		return OPTION_ID::LAST;
	}
	if(arg[1] == '-') {
		if(arg[2] == '\0') {
			return OPTION_ID::END;
		}
		const auto switch_long = std::string_view(arg + 2);
		for(const auto& opt : opts) {
			if(opt.switch_long == switch_long) {
				return opt.id;
			}
		}
	} else {
		if(arg[2] != '\0') {
			return OPTION_ID::ERROR;
		}
		for(const auto& opt : opts) {
			if(opt.switch_short == arg[1]) {
				return opt.id;
			}
		}
	}
	return OPTION_ID::ERROR;
}

// Prints the usage text and returns its error code.
REPLAY_CLI_RET Usage(
	int argc,
	const char *const *argv,
	const char *dat_basename,
	const char *dat_fn_default
)
{
	const auto opt_dat = std::ranges::find(OPTS, OPTION_ID::DAT, &OPTION::id);
	assert(opt_dat != OPTS.end());

	char *pack_param_desc = nullptr;
	const size_t pack_param_desc_len = SDL_asprintf(
		&pack_param_desc, "<%s>", dat_basename
	);
	defer(SDL_free(pack_param_desc));
	opt_dat->param_desc = { pack_param_desc, pack_param_desc_len };

	const char *prog_name = ((argc >= 1) ? argv[0] : "");
	const char *prog_space = ((argc >= 1) ? " " : "");
	SDL_printf(
		"Usage: %s%s[OPTIONS] replay_file [replay_file...]\n\n",
		prog_name,
		prog_space
	);

	SDL_printf("Options:\n");
	const auto max_len =
		(std::ranges::max)(std::views::transform(OPTS, [](const auto& opt) {
			return (opt.switch_long.length() + opt.param_desc.length());
		}));
	for(const auto& opt : OPTS) {
		const auto pad_len = (
			max_len - (opt.switch_long.length() + opt.param_desc.length())
		);
		SDL_printf(
			"  -%c, --%s %s%*s  %s",
			opt.switch_short,
			opt.switch_long.data(),
			opt.param_desc.data(),
			pad_len,
			"",
			opt.help.data()
		);
	}
	SDL_printf(
		"\n"
		"`replay_file` can contain `?` and `*` wildcards.\n"
		"If `-%c`/`--%s` is omitted, game data will be taken from `%s`.\n",
		opt_dat->switch_short,
		opt_dat->switch_long.data(),
		dat_fn_default
	);
	return REPLAY_CLI_RET::OK;
}

// Prints the version text and returns its error code.
REPLAY_CLI_RET Version(void)
{
	SDL_printf("CLI build: " VERSION_TAG "\n");
	SDL_printf("Game logic build: %s\n", VersionBuild());
	SDL_printf("Game logic version: %s\n", VersionLogic());
	return REPLAY_CLI_RET::OK;
}

REPLAY_CLI_RET ReplayCLI(int argc, char **argv, const char *dat_basename)
{
	// Redirect SDL's logger from `stderr` to `stdout`
#ifdef WIN32
	SetStdHandle(STD_ERROR_HANDLE, GetStdHandle(STD_OUTPUT_HANDLE));
#else
	dup2(STDOUT_FILENO, STDERR_FILENO);
#endif

	const auto path_data = PathForData();
	char *dat_fn_default = nullptr;
	SDL_asprintf(
		&dat_fn_default,
		"%s%s",
		std::bit_cast<const char *>(path_data.data()),
		dat_basename
	);
	defer(SDL_free(dat_fn_default));

	ARGS_GIVEN args = { .dat_fn = std::bit_cast<char8_t *>(dat_fn_default) };
	const auto run = [&](int positional_i) {
		const size_t count = (argc - positional_i);
		args.positionals = { (argv + positional_i), count };
		return ReplayCLI_Run(args);
	};

	int opt_i = 1;
	while(opt_i < argc) {
		switch(OptionFind(OPTS, argv[opt_i])) {
		case OPTION_ID::ERROR:
			return SDL_fatalf("unknown option: `%s`\n", argv[opt_i]);

		case OPTION_ID::LAST:
			return run(opt_i);

		case OPTION_ID::END:
			return run(opt_i + 1);

		case OPTION_ID::DAT:
			if(opt_i == (argc - 1)) {
				return SDL_fatalf("missing argument for `%s`\n", argv[opt_i]);
			}
			args.dat_fn = std::bit_cast<char8_t *>(argv[++opt_i]);
			break;

		case OPTION_ID::HELP:
			return Usage(argc, argv, dat_basename, dat_fn_default);

		case OPTION_ID::QUIET:
			args.quiet = true;
			break;

		case OPTION_ID::VERSION:
			return Version();
		}
		opt_i++;
	}
	return Usage(argc, argv, dat_basename, dat_fn_default);
}

// Returns a non-zero value on critical errors.
REPLAY_CLI_RET ReplayCLI_ProcessPositional(
	const char *arg_untyped, std::invocable<std::u8string_view> auto func
)
{
	const std::u8string_view arg = std::bit_cast<const char8_t *>(arg_untyped);
	const bool globbing = (arg.find_first_of(u8"*?") != decltype(arg)::npos);
	if(!globbing) {
		return func(arg);
	}

	// Adding 1 also turns `npos` to 0.
	const auto pattern_start = (arg.find_last_of(u8"/\\") + 1);

	const char *path = "./";
	char *path_buf = nullptr;
	defer(SDL_free(path_buf));
	if(pattern_start >= 1) {
		path_buf = SDL_strndup(arg_untyped, pattern_start);
		if(!path_buf) {
			return SDL_fatalf("%s: out of memory", arg_untyped);
		}
		path = path_buf;
	}

	auto **basenames = SDL_GlobDirectory(
		path, (arg_untyped + pattern_start), SDL_GLOB_CASEINSENSITIVE, nullptr
	);
	if(!basenames) {
		return SDL_fatalf("%s: out of memory", arg_untyped);
	}
	defer(SDL_free(basenames));

	auto ret = REPLAY_CLI_RET::OK;
	for(size_t i = 0; basenames[i] != nullptr; i++) {
		char *fn = nullptr;
		const size_t len = SDL_asprintf(&fn, "%s%s", path, basenames[i]);
		defer(SDL_free(fn));
		ret |= func(std::u8string_view{ std::bit_cast<char8_t *>(fn), len });
		if(!!(ret & REPLAY_CLI_RET::FATAL)) {
			return ret;
		}
	}
	return ret;
}
