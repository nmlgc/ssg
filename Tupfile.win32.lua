tup.include("libs/tupblocks/toolchain.msvc.lua")
tup.include("libs/SDL.lua")
tup.include("libs/xiph.lua")

local SDL_LINK = BuildSDL(CONFIG)
local XIPH_LINK = BuildXiph(CONFIG)

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
local blake3_i586_cfg = CONFIG:branch(BLAKE3_COMPILE, {
	cflags = {
		"/DBLAKE3_NO_SSE2", "/DBLAKE3_NO_AVX2", "/DBLAKE3_NO_AVX512"
	},
	objdir = "i586/",
})

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
	cc(blake3_modern_cfg, blake3_src) +
	cc(blake3_arch_cfgs[1], BLAKE3.join("blake3_sse2.c")) +
	cc(blake3_arch_cfgs[2], BLAKE3.join("blake3_avx2.c")) +
	cc(blake3_arch_cfgs[3], BLAKE3.join("blake3_avx512.c"))
)
local blake3_i586_obj = cc(blake3_i586_cfg, blake3_src)
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
	"/we26427", -- Static initialization order fiasco

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

local ssg_cfg = modules_cfg:branch(BLAKE3_LINK, XIPH_LINK, SSG_COMPILE, {
	cflags = {
		"/std:c++latest",
		"/DWIN32",
		"/EHsc",
		"/source-charset:utf-8",
		"/execution-charset:utf-8",
	},
})

-- Our platform layer code
LAYERS_SRC += SSG.glob("platform/miniaudio/*.cpp")
LAYERS_SRC += SSG.glob("platform/windows/*.cpp")
local layers_obj = cxx(ssg_cfg, LAYERS_SRC)

-- SDL code. The graphics backend is compiled separately to keep it switchable.
local p_sdl_cfg = ssg_cfg:branch(SDL_LINK, { lflags = "/SUBSYSTEM:windows" })
local p_sdl_src = (SSG.glob("platform/sdl/*.cpp") - { "graphics_sdl.cpp$" })
p_sdl_src += "MAIN/main_sdl.cpp"
local p_sdl_obj = cxx(p_sdl_cfg, p_sdl_src)

-- Main/regular build
local regular_obj = (
	cxx(ssg_cfg:branch(ANALYSIS_RELAXED), SSG_SRC) +
	layers_obj +
	p_sdl_obj +
	cxx(p_sdl_cfg, "platform/sdl/graphics_sdl.cpp") +
	blake3_modern_obj
)
exe(p_sdl_cfg, regular_obj, "GIAN07")
--
-- Vintage DirectDraw/Direct3D build
local p_vintage_cfg = p_sdl_cfg:branch(ANALYSIS_RELAXED, {
	cflags = "/DWIN32_VINTAGE", objdir = "vintage/",
})
local p_vintage_src = SSG.glob("platform/windows_vintage/DD*.CPP")
p_vintage_src += SSG.glob("platform/windows_vintage/D2_Polygon.CPP")
exe(
	p_vintage_cfg,
	(
		cxx(p_vintage_cfg, SSG_SRC) +
		layers_obj +
		p_sdl_obj +
		cxx(p_vintage_cfg, p_vintage_src) +
		blake3_i586_obj
	),
	"GIAN07 (original DirectDraw and Direct3D graphics)"
)
--- --------
