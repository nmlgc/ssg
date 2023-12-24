tup.include("libs/tupblocks/Tuprules.lua")

-- SDL
-- ---

SDL = sourcepath("libs/SDL/")
SDL_COMPILE = {
	base = {
		cflags = "/DDLL_EXPORT",
		lflags = (
			"advapi32.lib imm32.lib gdi32.lib kernel32.lib ole32.lib " ..
			"oleaut32.lib setupapi.lib user32.lib version.lib winmm.lib"
		),
		objdir = "SDL/",
	},
	buildtypes = {
		debug = { lflags = "ucrtd.lib" },
		release = { lflags = "ucrt.lib" },
	}
}
SDL_LINK = {
	base = {
		cflags = ("-I" .. SDL.join("include/")),
		lflags = "shell32.lib", -- for SDL_main()'s CommandLineToArgvW()
	},
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

sdl_cfg = CONFIG:branch("", SDL_COMPILE, SDL_LINK)
sdl_mslibc_cfg = sdl_cfg:branch("", {
	buildtypes = { release = { cflags = flag_remove("/GL") } }
})
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

XIPH_LINK = { base = { cflags = string.format(
	"-I%sinclude -I%sinclude", LIBOGG.root, LIBVORBIS.root
)} }
libogg_cfg = CONFIG:branch("", XIPH_LINK, { base = { objdir = "libogg/" } })
libogg_src += LIBOGG.glob("src/*.c")

libvorbis_cfg = CONFIG:branch("", XIPH_LINK, { base = {
	objdir = "libvorbis/"
} })
libvorbis_src += (LIBVORBIS.glob("lib/*.c") - {
	"barkmel.c$", "misc.c$", "psytune.c$", "tone.c$", "vorbisenc.c$"
})

xiph_obj = (cxx(libogg_cfg, libogg_src) + cxx(libvorbis_cfg, libvorbis_src))
-- --------------------

-- BLAKE3
-- ------

BLAKE3 = sourcepath("libs/BLAKE3/c/")
BLAKE3_COMPILE = {
	base = {
		objdir = "BLAKE3/",

		-- Visual C++ has no SSE4.1 flag, and I don't trust the /arch:AVX option
		cflags = "/DBLAKE3_NO_SSE41",
	}
}
BLAKE3_LINK = { base = { cflags = ("-I" .. BLAKE3.root) } }

blake3_src += BLAKE3.join("blake3.c")
blake3_src += BLAKE3.join("blake3_dispatch.c")
blake3_src += BLAKE3.join("blake3_portable.c")

blake3_modern_cfg = CONFIG:branch("", BLAKE3_COMPILE, { base = {
	objdir = "modern/",
} })

-- Each optimized version must be built with the matching Visual C++ `/arch`
-- flag. This is not only necessary for the compiler to actually emit the
-- intended instructions, but also prevents newer instruction sets from
-- accidentally appearing in older code. (For example, globally setting
-- `/arch:AVX512` for all of these files would cause AVX-512 instructions to
-- also appear in the AVX2 version, breaking it on those CPUs.)
-- That's why they recommend the ASM versions, but they're 64-bit-exclusive…
blake3_arch_cfgs = {
	blake3_modern_cfg:branch("", { base = { cflags = "/arch:SSE2" } }),
	blake3_modern_cfg:branch("", { base = { cflags = "/arch:AVX2" } }),
	blake3_modern_cfg:branch("", { base = { cflags = "/arch:AVX512" } }),
}
blake3_modern_obj = (
	cxx(blake3_modern_cfg, blake3_src) +
	cxx(blake3_arch_cfgs[1], BLAKE3.join("blake3_sse2.c")) +
	cxx(blake3_arch_cfgs[2], BLAKE3.join("blake3_avx2.c")) +
	cxx(blake3_arch_cfgs[3], BLAKE3.join("blake3_avx512.c"))
)
-- ------

-- Static analysis using the C++ Core Guideline checker plugin.
ANALYSIS_CFLAGS = (
	"/analyze:autolog- /analyze:plugin EspXEngine.dll " ..
	"/external:W0 /external:anglebrackets /analyze:external- " ..

	-- Critical warnings
	"/we26819 " .. -- Unannotated fallthrough between switch labels
	"/we26427 " .. -- Static initialization order fiasco

	-- Disabled warnings
	"/wd4834 " .. -- Discarding `[[nodiscard]]` (C6031 covers this and more)
	"/wd26408 " .. -- Avoid _malloca()
	"/wd26432 " .. -- Rule of Five boilerplate
	"/wd26440 " .. -- `noexcept` all the things
	"/wd26481 " .. -- Don't use pointer arithmetic
	"/wd26482 " .. -- Only index into arrays using constant expressions
	"/wd26490 " .. -- Don't use `reinterpret_cast`
	"/wd26429 /wd26446 /wd26472 /wd26821" -- Guideline Support Library
)

ANALYSIS = {
	buildtypes = {
		release = { cflags = ANALYSIS_CFLAGS },
	},
}

main_cfg = CONFIG:branch(tup.getconfig("BUILDTYPE"), SDL_LINK, BLAKE3_LINK, {
	base = {
		cflags = (
			"/std:c++latest " ..
			"/DWIN32 " ..
			"/I. " ..
			"/EHsc " ..
			"/source-charset:utf-8 " ..
			"/execution-charset:utf-8"
		),
		lflags = "obj/dinput.lib",
		objdir = "ssg/",
	},
	buildtypes = {
		debug = { cflags = "/DPBG_DEBUG" },
	}
})

modern_cfg = main_cfg:branch(tup.getconfig("BUILDTYPE"), ANALYSIS)
modern_src += tup.glob("game/*.cpp")
modern_src += "game/codecs/flac.cpp"
modern_src += tup.glob("platform/windows/*.cpp")
main_src += tup.glob("DirectXUTYs/*.CPP")
main_src += tup.glob("DirectXUTYs/*.cpp")
main_src += tup.glob("GIAN07/*.cpp")
main_src += tup.glob("GIAN07/*.CPP")

main_obj = (
	cxx(modern_cfg, modern_src) +
	cxx(modern_cfg:branch("", XIPH_LINK), "game/codecs/vorbis.cpp") +
	cxx(main_cfg, main_src)
)

main_sdl_cfg = main_cfg:branch("", ANALYSIS, {
	base = { lflags = "/SUBSYSTEM:windows" }
})

main_sdl_src += "MAIN/main_sdl.cpp"
main_sdl_src += tup.glob("platform/miniaudio/*.cpp")
main_sdl_src += tup.glob("platform/sdl/*.cpp")
main_sdl_obj = cxx(main_sdl_cfg, main_sdl_src)
main_sdl_obj = (main_sdl_obj + xiph_obj)
exe(
	main_sdl_cfg,
	(main_sdl_obj + main_obj + blake3_modern_obj + sdl_dll),
	"GIAN07"
)
