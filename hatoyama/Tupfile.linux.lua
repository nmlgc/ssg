tup.import("TOOLCHAIN=gcc")
tup.include("libs/tupblocks/toolchain." .. TOOLCHAIN .. ".lua")
tup.include("libs/BLAKE3.lua")

---@param constants_cflags? ConfigVarBuildtyped<string> Contains the include path of `constants.h`
function BuildHatoyama(constants_cflags)
	local PLATFORM_LINK = EnvConfig("sdl3", "pangocairo", "fontconfig")
	local LAYERS_LINK = EnvConfig("libwebp", "ogg", "vorbis", "vorbisfile")
	local BLAKE3_LINK = (EnvConfig("libblake3") or BuildBLAKE3(CONFIG, 0))

	local link = { cflags = { "-DLINUX" } }
	link.cflags += constants_cflags
	TableExtend(link, HATOYAMA_LINK)

	-- Since Pango/Cairo adds -pthread to a later configuration, the C++
	-- standard library must also be compiled with this flag.
	local base_cfg = CONFIG:branch({ cflags = "-pthread" })

	local game_cfg = CONFIG:branch(
		BLAKE3_LINK, LAYERS_LINK, base_cfg:cxx_std_modules(), link
	)
	local platform_cfg = game_cfg:branch(PLATFORM_LINK)

	local game_src = (HATOYAMA.glob("platform/c/*.cpp"))
	game_src += HATOYAMA_SRC

	local platform_src = HATOYAMA.glob("platform/sdl/*.cpp")
	platform_src += HATOYAMA.glob("platform/miniaudio/*.cpp")
	platform_src += HATOYAMA.glob("platform/pangocairo/*.cpp")

	-- Clang does not like C being compiled with clang++, and non-C++ clang
	-- does not like module-related switches.
	local c_src = HATOYAMA.glob("platform/miniaudio/*.c")

	local obj = (
		game_cfg:branch(HATOYAMA_COMPILE):cxx(game_src) +
		platform_cfg:branch(HATOYAMA_COMPILE):cxx(platform_src) +
		base_cfg:branch(HATOYAMA_COMPILE, link):cc(c_src)
	)
	return platform_cfg:branch({ linputs = obj })
end
