tup.import("TOOLCHAIN=gcc")
tup.include("libs/tupblocks/toolchain." .. TOOLCHAIN .. ".lua")
tup.include("libs/BLAKE3.lua")
tup.include("libs/printf.lua")

-- We need this library in a non-C++ build step later.
local PRINTF_LINK = BuildPrintf(CONFIG)

---@param constants_cflags ConfigVarBuildtyped<string> Contains the include path of `constants.h`
function BuildHatoyamaLogic(constants_cflags)
	local link = { cflags = { "-DLINUX" } }
	link.cflags += constants_cflags
	TableExtend(link, HATOYAMA_LINK)

	-- Since Pango/Cairo adds -pthread to a later configuration, the C++
	-- standard library must also be compiled with this flag.
	local modules_cfg = CONFIG:branch({ cflags = "-pthread" })

	local dep_cfg = modules_cfg:branch(link)
	local link_cfg = dep_cfg:branch(modules_cfg:cxx_std_modules(), PRINTF_LINK)

	local src
	src += HATOYAMA_LOGIC.src
	src += HATOYAMA.glob("logic/c/*.cpp")

	local obj = link_cfg:branch(HATOYAMA_LOGIC.compile):cxx(src)
	return dep_cfg, link_cfg:branch({ linputs = obj })
end

---@param dep_cfg Config
---@param logic_cfg Config
function BuildHatoyamaEngine(dep_cfg, logic_cfg)
	local LIBS_LINK = EnvConfig(
		"fontconfig",
		"libwebp",
		"ogg",
		"pangocairo",
		"sdl3",
		"vorbis",
		"vorbisfile"
	)
	local BLAKE3_LINK = (EnvConfig("libblake3") or BuildBLAKE3(CONFIG, 0))

	local link_cfg = logic_cfg:branch(LIBS_LINK, BLAKE3_LINK)

	local src
	src += HATOYAMA_ENGINE.src
	src += HATOYAMA.glob("engine/c/*.cpp")
	src += HATOYAMA.glob("engine/miniaudio/*.cpp")
	src += HATOYAMA.glob("engine/pangocairo/*.cpp")
	src += HATOYAMA.glob("engine/sdl/*.cpp")

	-- Clang does not like C being compiled with clang++, and non-C++ clang
	-- does not like module-related switches.
	local c_src = HATOYAMA.glob("engine/miniaudio/*.c")

	local obj = (
		link_cfg:branch(HATOYAMA_ENGINE.compile):cxx(src) +
		dep_cfg:branch(HATOYAMA_ENGINE.compile, PRINTF_LINK):cc(c_src)
	)
	return link_cfg:branch({ linputs = obj })
end
