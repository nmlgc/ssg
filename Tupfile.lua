tup.include("libs/tupblocks/Tuprules.lua")

-- SDL
-- ---

SDL = sourcepath("libs/SDL/")
SDL_COMPILE = {
	cflags = "/DDLL_EXPORT",
	lflags = {
		"advapi32.lib",
		"imm32.lib",
		"gdi32.lib",
		"kernel32.lib",
		"ole32.lib",
		"oleaut32.lib",
		"setupapi.lib",
		"user32.lib",
		"version.lib",
		"winmm.lib",
		debug = "ucrtd.lib",
		release = "ucrt.lib",
	},
	objdir = "SDL/",
}
SDL_LINK = {
	cflags = ("-I" .. SDL.join("include/")),
	lflags = "shell32.lib", -- for SDL_main()'s CommandLineToArgvW()
}

sdl_src += SDL.glob("src/*.c")
sdl_src += SDL.glob("src/atomic/*.c")
sdl_src += (SDL.glob("src/audio/*.c") - { "SDL_audiodev.c$" })
sdl_src += SDL.glob("src/audio/directsound/*.c")
sdl_src += SDL.glob("src/audio/disk/*.c")
sdl_src += SDL.glob("src/audio/dummy/*.c")
sdl_src += SDL.glob("src/audio/winmm/*.c")
sdl_src += (SDL.glob("src/audio/wasapi/*.c") - { "winrt.cpp$" })
sdl_src += SDL.glob("src/core/windows/*.c")
sdl_src += SDL.glob("src/cpuinfo/*.c")
sdl_src += SDL.glob("src/dynapi/*.c")
sdl_src += (
	SDL.glob("src/events/*.c") -
	{ "imKStoUCS.c$", "SDL_keysym_to_scancode.c$", "SDL_scancode_tables.c$" }
)
sdl_src += SDL.glob("src/file/*.c")
sdl_src += SDL.glob("src/filesystem/windows/*.c")
sdl_src += SDL.glob("src/haptic/*.c")
sdl_src += SDL.glob("src/haptic/windows/*.c")
sdl_src += SDL.glob("src/hidapi/*.c")
sdl_src += SDL.glob("src/joystick/*.c")
sdl_src += SDL.glob("src/joystick/hidapi/*.c")
sdl_src += SDL.glob("src/joystick/virtual/*.c")
sdl_src += SDL.glob("src/joystick/windows/*.c")
sdl_src += SDL.glob("src/libm/*.c")
sdl_src += SDL.glob("src/loadso/windows/*.c")
sdl_src += SDL.glob("src/locale/*.c")
sdl_src += SDL.glob("src/locale/windows/*.c")
sdl_src += SDL.glob("src/misc/*.c")
sdl_src += SDL.glob("src/misc/windows/*.c")
sdl_src += SDL.glob("src/power/*.c")
sdl_src += SDL.glob("src/power/windows/*.c")
sdl_src += SDL.glob("src/render/*.c")
sdl_src += SDL.glob("src/render/direct3d/*.c")
sdl_src += (SDL.glob("src/render/direct3d11/*.c") - { "winrt.cpp$" })
sdl_src += (SDL.glob("src/render/direct3d12/*.c") - { "winrt.cpp$" })
sdl_src += SDL.glob("src/render/opengl/*.c")
sdl_src += SDL.glob("src/render/opengles2/*.c")
sdl_src += SDL.glob("src/render/software/*.c")
sdl_src += SDL.glob("src/sensor/*.c")
sdl_src += SDL.glob("src/sensor/windows/*.c")
sdl_src += (SDL.glob("src/stdlib/*.c") - { "SDL_mslibc.c$" })
sdl_src += SDL.glob("src/thread/*.c")
sdl_src += SDL.glob("src/thread/windows/*.c")
sdl_src += SDL.glob("src/timer/*.c")
sdl_src += SDL.glob("src/timer/windows/*.c")
sdl_src += SDL.glob("src/video/*.c")
sdl_src += SDL.glob("src/video/dummy/*.c")
sdl_src += SDL.glob("src/video/windows/*.c")
sdl_src += SDL.glob("src/video/yuv2rgb/*.c")
sdl_src += SDL.join("src/thread/generic/SDL_syscond.c")
sdl_winmain_src += SDL.glob("src/main/windows/*.c")

sdl_cfg = CONFIG:branch(SDL_COMPILE, SDL_LINK)
sdl_mslibc_cfg = sdl_cfg:branch({ cflags = { release = flag_remove("/GL") } })
sdl_obj = (
	cxx(sdl_cfg, sdl_src) +
	cxx(sdl_mslibc_cfg, SDL.join("src/stdlib/SDL_mslibc.c")) +
	rc(sdl_cfg, SDL.join("src/main/windows/version.rc"))
)
sdl_dll = (
	dll(sdl_cfg, sdl_obj, "SDL2") +
	cxx(sdl_cfg, sdl_winmain_src)
)
-- ---

-- libogg and libvorbis
-- --------------------

LIBOGG = sourcepath("libs/libogg/")
LIBVORBIS = sourcepath("libs/libvorbis/")

XIPH_LINK = { cflags = string.format(
	"-I%sinclude -I%sinclude", LIBOGG.root, LIBVORBIS.root
)}
local libogg_cfg = CONFIG:branch(XIPH_LINK, { objdir = "libogg/" })
libogg_src += LIBOGG.glob("src/*.c")

local libvorbis_cfg = CONFIG:branch(XIPH_LINK, { objdir = "libvorbis/" })
libvorbis_src += (LIBVORBIS.glob("lib/*.c") - {
	"barkmel.c$", "misc.c$", "psytune.c$", "tone.c$", "vorbisenc.c$"
})

xiph_obj = (cxx(libogg_cfg, libogg_src) + cxx(libvorbis_cfg, libvorbis_src))
-- --------------------

-- BLAKE3
-- ------

BLAKE3 = sourcepath("libs/BLAKE3/c/")
BLAKE3_COMPILE = {
	objdir = "BLAKE3/",

	-- Visual C++ has no SSE4.1 flag, and I don't trust the /arch:AVX option
	cflags = "/DBLAKE3_NO_SSE41",
}
BLAKE3_LINK = { cflags = ("-I" .. BLAKE3.root) }

blake3_src += BLAKE3.join("blake3.c")
blake3_src += BLAKE3.join("blake3_dispatch.c")
blake3_src += BLAKE3.join("blake3_portable.c")

local blake3_modern_cfg = CONFIG:branch(BLAKE3_COMPILE, { objdir = "modern/" })

-- Each optimized version must be built with the matching Visual C++ `/arch`
-- flag. This is not only necessary for the compiler to actually emit the
-- intended instructions, but also prevents newer instruction sets from
-- accidentally appearing in older code. (For example, globally setting
-- `/arch:AVX512` for all of these files would cause AVX-512 instructions to
-- also appear in the AVX2 version, breaking it on those CPUs.)
-- That's why they recommend the ASM versions, but they're 64-bit-exclusive…
blake3_arch_cfgs = {
	blake3_modern_cfg:branch({ cflags = "/arch:SSE2" }),
	blake3_modern_cfg:branch({ cflags = "/arch:AVX2" }),
	blake3_modern_cfg:branch({ cflags = "/arch:AVX512" }),
}
blake3_modern_obj = (
	cxx(blake3_modern_cfg, blake3_src) +
	cxx(blake3_arch_cfgs[1], BLAKE3.join("blake3_sse2.c")) +
	cxx(blake3_arch_cfgs[2], BLAKE3.join("blake3_avx2.c")) +
	cxx(blake3_arch_cfgs[3], BLAKE3.join("blake3_avx512.c"))
)
-- ------

-- Static analysis using the C++ Core Guideline checker plugin.
local ANALYSIS = { cflags = { release = {
	"/analyze:autolog-",
	"/analyze:plugin EspXEngine.dll",
	"/external:W0",
	"/external:anglebrackets",
	"/analyze:external-",

	-- Critical warnings
	"/we26819", -- Unannotated fallthrough between switch labels

	-- Opt-in warnings
	"/w14101", -- Unreferenced local variable

	-- Disabled warnings
	"/wd4834", -- Discarding `[[nodiscard]]` (C6031 covers this and more)
	"/wd26408", -- Avoid _malloca()
	"/wd26432", -- Rule of Five boilerplate
	"/wd26440", -- `noexcept` all the things
	"/wd26481", -- Don't use pointer arithmetic
	"/wd26482", -- Only index into arrays using constant expressions
	"/wd26490", -- Don't use `reinterpret_cast`
	"/wd26429", -- Guideline Support Library
	"/wd26446", -- …
	"/wd26472", -- …
	"/wd26821", -- …
	"/wd26427", -- Static initialization order fiasco (for now)
} } }

-- Relaxed analysis flags for pbg code
local ANALYSIS_RELAXED = { cflags = { release = {
	"/wd6246", -- Hiding declarations in outer scope
	"/wd26438", -- Avoid 'goto'
	"/wd26448", -- …
	"/wd26450", -- Compile-time overflows (always intended)
	"/wd26485", -- No array to pointer decay
	"/wd26494", -- Uninitialized variables
	"/wd26495", -- Uninitialized member variables
	"/wd26818", -- Switch statement does not cover all cases
} } }

local modules_cfg = CONFIG:branch(ANALYSIS)
modules_cfg = modules_cfg:branch(cxx_std_modules(modules_cfg))

--- The game
-- --------

local SSG = sourcepath("./")
local ssg_cfg = modules_cfg:branch(BLAKE3_LINK, {
	cflags = {
		"/std:c++latest",
		"/DWIN32",
		"/I.",
		"/IGIAN07/",
		"/EHsc",
		"/source-charset:utf-8",
		"/execution-charset:utf-8",
		debug = "/DPBG_DEBUG",
	},
	objdir = "ssg/",
})

-- pbg code
local ssg_src
ssg_src += SSG.glob("DirectXUTYs/*.CPP")
ssg_src += SSG.glob("DirectXUTYs/*.cpp")
ssg_src += SSG.glob("GIAN07/*.cpp")
ssg_src += SSG.glob("GIAN07/*.CPP")

-- Our platform layer code
local layers_src = "game/codecs/flac.cpp"
layers_src += SSG.glob("game/*.cpp")
layers_src += SSG.glob("platform/miniaudio/*.cpp")
layers_src += SSG.glob("platform/windows/*.cpp")
local layers_obj = (
	cxx(ssg_cfg, layers_src) +
	cxx(ssg_cfg:branch(XIPH_LINK), "game/codecs/vorbis.cpp")
)

-- SDL code
local p_sdl_cfg = ssg_cfg:branch(SDL_LINK, { lflags = "/SUBSYSTEM:windows" })
local p_sdl_src = SSG.glob("platform/sdl/*.cpp")
p_sdl_src += "MAIN/main_sdl.cpp"
local p_sdl_obj = cxx(p_sdl_cfg, p_sdl_src)

-- Main/regular build
local regular_obj = (
	cxx(ssg_cfg:branch(ANALYSIS_RELAXED), ssg_src) +
	layers_obj +
	p_sdl_obj +
	xiph_obj +
	blake3_modern_obj +
	sdl_dll
)
exe(p_sdl_cfg, regular_obj, "GIAN07")
--- --------
