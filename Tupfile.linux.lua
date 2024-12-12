tup.include("libs/tupblocks/toolchain.clang.lua")
tup.include("libs/BLAKE3.lua")

local PLATFORM_LINK = EnvConfig("sdl2", "pangocairo")
local XIPH_LINK = EnvConfig("ogg", "vorbis", "vorbisfile")
local BLAKE3_LINK = (EnvConfig("libblake3") or BuildBLAKE3(CONFIG, 0))

-- Since Pango/Cairo adds -pthread to a later configuration, the C++ standard
-- library must also be compiled with this flag.
CONFIG = CONFIG:branch({ cflags = "-pthread" })

local ssg_cfg = CONFIG:branch(
	BLAKE3_LINK, XIPH_LINK, SSG_COMPILE, cxx_std_modules(CONFIG), {
		cflags = { "-DLINUX" },
	}
)
local ssg_obj = cxx(ssg_cfg, SSG_SRC)

-- Our platform layer code
LAYERS_SRC += (SSG.glob("platform/c/*.cpp"))
ssg_obj = (ssg_obj + cxx(ssg_cfg, LAYERS_SRC))

local platform_cfg = ssg_cfg:branch(PLATFORM_LINK)
local platform_src = SSG.glob("platform/sdl/*.cpp")
platform_src += SSG.glob("platform/miniaudio/*.cpp")
platform_src += SSG.glob("platform/pangocairo/*.cpp")
platform_src += "MAIN/main_sdl.cpp"
ssg_obj = (ssg_obj + cxx(platform_cfg, platform_src))

exe(platform_cfg, ssg_obj, "GIAN07")
