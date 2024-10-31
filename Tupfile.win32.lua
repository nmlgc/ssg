tup.include("libs/tupblocks/toolchain.msvc.lua")
tup.include("libs/BLAKE3.lua")
tup.include("libs/SDL.lua")
tup.include("libs/xiph.lua")

local SDL_LINK = BuildSDL(CONFIG)
local XIPH_LINK = BuildXiph(CONFIG)

MODERN = 0
VINTAGE = 1

---@param variant integer
local function ssg(variant)
	local variant_cfg
	local variant_bin_suffix = ""
	if (variant == MODERN) then
		variant_cfg = CONFIG:branch({ objdir = "modern/" })
	elseif (variant == VINTAGE) then
		variant_cfg = CONFIG:branch({
			cflags = {
				"/DWIN32_VINTAGE",
			},
			objdir = "vintage/"
		})
		variant_bin_suffix = " (original DirectDraw and Direct3D graphics)"
	end

	local BLAKE3_LINK = BuildBLAKE3(variant_cfg, variant)

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

	local modules_cfg = variant_cfg:branch(ANALYSIS)
	modules_cfg = modules_cfg:branch(cxx_std_modules(modules_cfg))

	-- The game
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
	local ssg_obj = cxx(ssg_cfg:branch(ANALYSIS_RELAXED), SSG_SRC)

	-- Our platform layer code
	LAYERS_SRC += SSG.glob("platform/miniaudio/*.cpp")
	LAYERS_SRC += SSG.glob("platform/windows/*.cpp")
	ssg_obj = (ssg_obj + cxx(ssg_cfg, LAYERS_SRC))

	local platform_cfg = ssg_cfg:branch(SDL_LINK, {
		lflags = "/SUBSYSTEM:windows"
	})
	local p_modern_src = (
		SSG.glob("platform/sdl/*.cpp") - { "graphics_sdl.cpp$" }
	)
	if (variant == MODERN) then
		p_modern_src += "platform/sdl/graphics_sdl.cpp"
	end
	p_modern_src += "MAIN/main_sdl.cpp"
	ssg_obj = (ssg_obj + cxx(platform_cfg, p_modern_src))

	if (variant == VINTAGE) then
		local p_vintage_cfg = platform_cfg:branch(ANALYSIS_RELAXED)
		local p_vintage_src = SSG.glob("platform/windows_vintage/DD*.CPP")
		p_vintage_src += SSG.glob("platform/windows_vintage/D2_Polygon.CPP")
		ssg_obj = (ssg_obj + cxx(p_vintage_cfg, p_vintage_src))
	end

	exe(platform_cfg, ssg_obj, ("GIAN07" .. variant_bin_suffix))
end

ssg(MODERN)
ssg(VINTAGE)
