tup.include("libs/tupblocks/toolchain.msvc.lua")
tup.include("libs/BLAKE3.lua")
tup.include("libs/SDL.lua")
tup.include("libs/xiph.lua")

local XIPH_LINK = BuildXiph(CONFIG)

MODERN = 0
VINTAGE = 1

-- Icon
local ssg_ico = CONFIG:rc(SSG.join("GIAN07/GIAN07.rc"))

---@param variant integer
local function ssg(variant)
	local sdl_version = 3
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
		sdl_version = 2
	end

	local SDL_LINK = BuildSDL(variant_cfg, sdl_version)
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
	modules_cfg = modules_cfg:branch(modules_cfg:cxx_std_modules())

	-- The game
	-- --------

	local ssg_cfg = modules_cfg:branch(
		BLAKE3_LINK, XIPH_LINK, SDL_LINK, SSG_COMPILE, {
			cflags = {
				"/std:c++latest",
				"/DWIN32",
				"/EHsc",
				"/source-charset:utf-8",
				"/execution-charset:utf-8",
			},
			lflags = "/SUBSYSTEM:windows",
		}
	)
	local ssg_obj = ssg_cfg:branch(ANALYSIS_RELAXED):cxx(SSG_SRC)

	-- Our platform layer code
	LAYERS_SRC += SSG.glob("platform/miniaudio/*.c*")
	LAYERS_SRC += SSG.glob("platform/windows/*.cpp")
	ssg_obj = (ssg_obj + ssg_cfg:cxx(LAYERS_SRC))

	local p_modern_src = (
		SSG.glob("platform/sdl/*.cpp") - { "graphics_sdl.cpp$" }
	)
	if (variant == MODERN) then
		p_modern_src += "platform/sdl/graphics_sdl.cpp"
	end
	p_modern_src += "MAIN/main_sdl.cpp"
	p_modern_src.extra_inputs += PLATFORM_CONSTANTS
	ssg_obj = (ssg_obj + ssg_cfg:cxx(p_modern_src))

	if (variant == VINTAGE) then
		local p_vintage_cfg = ssg_cfg:branch(ANALYSIS_RELAXED)
		local p_vintage_src = SSG.glob("platform/windows_vintage/DD*.CPP")
		p_vintage_src += SSG.glob("platform/windows_vintage/D2_Polygon.CPP")
		ssg_obj = (ssg_obj + p_vintage_cfg:cxx(p_vintage_src))
	end

	ssg_obj = (ssg_obj + ssg_ico)
	ssg_cfg:exe(ssg_obj, ("GIAN07" .. variant_bin_suffix))
end

ssg(MODERN)
ssg(VINTAGE)
