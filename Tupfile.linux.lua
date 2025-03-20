tup.include("libs/tupblocks/toolchain.clang.lua")
tup.include("libs/BLAKE3.lua")

tup.import("SDL")
SDL = SDL[1]

local PLATFORM_LINK = EnvConfig(SDL, "pangocairo", "fontconfig")
local LAYERS_LINK = EnvConfig("libwebp", "ogg", "vorbis", "vorbisfile")
local BLAKE3_LINK = (EnvConfig("libblake3") or BuildBLAKE3(CONFIG, 0))

-- Since Pango/Cairo adds -pthread to a later configuration, the C++ standard
-- library must also be compiled with this flag.
CONFIG = CONFIG:branch({ cflags = "-pthread" })

local ssg_cfg = CONFIG:branch(
	BLAKE3_LINK, LAYERS_LINK, SSG_COMPILE, CONFIG:cxx_std_modules(), {
		cflags = { "-DLINUX", ("-D" .. string.upper(SDL) .. "=1") },
	}
)
local ssg_obj = ssg_cfg:cxx(SSG_SRC)

-- Our platform layer code
LAYERS_SRC += (SSG.glob("platform/c/*.cpp"))
ssg_obj = (ssg_obj + ssg_cfg:cxx(LAYERS_SRC))

local platform_cfg = ssg_cfg:branch(PLATFORM_LINK)
local platform_src = SSG.glob("platform/sdl/*.cpp")
platform_src += SSG.glob("platform/miniaudio/*.cpp")
platform_src += SSG.glob("platform/pangocairo/*.cpp")
platform_src += "MAIN/main_sdl.cpp"
platform_src.extra_inputs += PLATFORM_CONSTANTS
ssg_obj = (
	ssg_obj +
	platform_cfg:cxx(platform_src) +

	-- Clang does not like C being compiled with clang++, and non-C++ clang
	-- does not like module-related switches.
	CONFIG:branch(SSG_COMPILE):cc(SSG.glob("platform/miniaudio/*.c"))
)

platform_cfg:exe(ssg_obj, "GIAN07")
