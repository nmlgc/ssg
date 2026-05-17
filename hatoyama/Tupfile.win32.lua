tup.include("libs/tupblocks/toolchain.msvc.lua")
tup.include("libs/9xcompat.lua")
tup.include("libs/BLAKE3.lua")
tup.include("libs/libwebp_lossless.lua")
tup.include("libs/SDL.lua")
tup.include("libs/xiph.lua")

-- Variants
-- --------

MODERN = 0
VINTAGE = 1

---@param variant 0 | 1
function VariantBinSuffix(variant)
	return ({
		[MODERN] = "",
		[VINTAGE] = "_win98"
	})[variant]
end
-- --------

-- Static analysis using the C++ Core Guideline checker plugin.
ANALYSIS = { cflags = { release = {
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
ANALYSIS_RELAXED = { cflags = { release = {
	"/wd6246", -- Hiding declarations in outer scope
	"/wd26438", -- Avoid 'goto'
	"/wd26448", -- …
	"/wd26450", -- Compile-time overflows (always intended)
	"/wd26485", -- No array to pointer decay
	"/wd26494", -- Uninitialized variables
	"/wd26495", -- Uninitialized member variables
	"/wd26818", -- Switch statement does not cover all cases
} } }

---@param variant integer
---@param constants_cflags? ConfigVarBuildtyped<string> Contains the include path of `constants.h`
function BuildHatoyama(variant, constants_cflags)
	local dep_cfg
	if (variant == MODERN) then
		dep_cfg = CONFIG:branch({
			cflags = {
				-- WebP only uses multithreading for effort levels 8 and 9,
				-- where it does significantly boost performance.
				"-DWEBP_USE_THREAD",
			},
			objdir = "modern/",
		})
	elseif (variant == VINTAGE) then
		dep_cfg = CONFIG:branch({
			cflags = {
				"/DWIN32_VINTAGE",
				"/D_WIN32_WINNT=0x0400", -- needed for libwebp
				"/D__WIN9X__",
				"/arch:IA32",
				"/Zc:threadSafeInit-",
			},
			-- Saves 512 to 1024 bytes!
			lflags = { FlagRemove("/MANIFEST:.*"), "/MANIFEST:NO" },

			objdir = "vintage/",
		})
	end

	local XIPH_LINK = BuildXiph(dep_cfg)
	local SDL_LINK = BuildSDL(dep_cfg, VariantBinSuffix(variant))
	local BLAKE3_LINK = BuildBLAKE3(dep_cfg, variant)
	local LIBWEBP_LINK = BuildLibWebPLosslessEncode(dep_cfg, variant)

	local modules_cfg = dep_cfg:branch(ANALYSIS)
	modules_cfg = modules_cfg:branch(modules_cfg:cxx_std_modules())

	local link = {
		cflags = {
			"/std:c++latest",
			"/DWIN32",
			"/EHsc",
			"/source-charset:utf-8",
			"/execution-charset:utf-8",
		},
		lflags = "/SUBSYSTEM:windows",
	}
	link.cflags += constants_cflags
	TableExtend(link, HATOYAMA_LINK)

	local link_cfg = modules_cfg:branch(
		BLAKE3_LINK, LIBWEBP_LINK, XIPH_LINK, SDL_LINK, link
	)
	local compile_cfg = link_cfg:branch(HATOYAMA_COMPILE)

	local src
	src += HATOYAMA_SRC
	src += HATOYAMA.glob("platform/miniaudio/*.c*")
	src += HATOYAMA.glob("platform/windows/*.cpp")
	src += (HATOYAMA.glob("platform/sdl/*.cpp") - { "graphics_sdl.cpp$" })
	if (variant == MODERN) then
		src += HATOYAMA.glob("platform/sdl/graphics_sdl.cpp")
	end
	local obj = compile_cfg:cxx(src)

	if (variant == VINTAGE) then
		local vintage_cfg = compile_cfg:branch(ANALYSIS_RELAXED)
		local vintage_src = HATOYAMA.glob("platform/windows_vintage/DD*.CPP")
		vintage_src += HATOYAMA.glob("platform/windows_vintage/D2_Polygon.CPP")

		obj = (obj + vintage_cfg:cxx(vintage_src))
	end

	return dep_cfg, link_cfg:branch({ linputs = obj })
end
