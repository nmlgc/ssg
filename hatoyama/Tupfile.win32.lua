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
---@param constants_cflags ConfigVarBuildtyped<string> Contains the include path of `constants.h`
function BuildHatoyamaLogic(variant, constants_cflags)
	local dep_cfg
	if (variant == MODERN) then
		dep_cfg = CONFIG:branch({ objdir = "modern/" })
	elseif (variant == VINTAGE) then
		dep_cfg = CONFIG:branch({
			cflags = {
				-- Must be a link flag, since it's needed by `SDL_thread.h`
				-- to opt out of `_beginthreadex()` and `_endthreadex()`.
				"/D__WIN9X__",

				"/DWIN32_VINTAGE",
				"/arch:IA32",
				"/Zc:threadSafeInit-",
			},
			-- Saves 512 to 1024 bytes!
			lflags = { FlagRemove("/MANIFEST:.*"), "/MANIFEST:NO" },

			objdir = "vintage/",
		})
	end

	-- Opt out of exception unwinding for the C++ standard library to avoid
	-- references to modern system APIs for the vintage build.
	-- This makes sense in general though, because it's always safe: Even
	-- though the entirety of the standard library throws plenty of exceptions,
	-- all `std` module code is either part of a template or an `inline`
	-- function, and is therefore re-instantiated in every translation unit
	-- that uses the respective function – and there, only the translation
	-- unit's own `/EH` flag counts, not this one.
	local std_compile = { cflags = {
		release = { FlagRemove("/EHsc"), "/wd4530" }
	} }
	local modules_cfg = dep_cfg:branch(ANALYSIS)
	modules_cfg = modules_cfg:branch(modules_cfg:cxx_std_modules(std_compile))

	-- As long as we don't catch any exceptions on our own, we can safely
	-- compile without `/EHsc`. Any exception would immediately terminate the
	-- process anyway, regardless of whether we opted into stack unwinding or
	-- not. If we do `catch` without `/EHsc`, MSVC raises C4530 in any case
	-- where the missing stack unwinding would actually matter – and treating
	-- this warning as an error protects us from accidentally creating such a
	-- case.
	local link = {
		cflags = {
			"/std:c++latest",
			"/DWIN32",
			"/source-charset:utf-8",
			"/execution-charset:utf-8",
			release = { FlagRemove("/wd4530"), "/we4530" },
		},
	}
	link.cflags += constants_cflags
	TableExtend(link, HATOYAMA_LINK)

	local src
	src += HATOYAMA_LOGIC.src

	local link_cfg = modules_cfg:branch(link)
	local compile_cfg = link_cfg:branch(HATOYAMA_LOGIC.compile)
	local obj = compile_cfg:cxx(src)
	return dep_cfg, link_cfg:branch({
		linputs = obj,
	})
end

---@param dep_cfg Config
---@param logic_cfg Config
---@param variant integer
function BuildHatoyamaEngine(dep_cfg, logic_cfg, variant)
	---@type ConfigShape
	local libwebp_compile = {}
	if (variant == MODERN) then
		libwebp_compile.cflags = {
			-- WebP only uses multithreading for effort levels 8 and 9,
			-- where it does significantly boost performance.
			"-DWEBP_USE_THREAD",
		}
	elseif (variant == VINTAGE) then
		libwebp_compile.cflags = { "/D_WIN32_WINNT=0x0400" }
	end
	local libwebp_cfg = dep_cfg:branch(libwebp_compile)

	---@type ConfigShape
	local sdl_compile = {}
	if (variant == VINTAGE) then
		sdl_compile.cflags = { "/D_WIN32_WINNT=0x0400" }
	end
	local sdl_cfg = dep_cfg:branch(sdl_compile)

	local XIPH_LINK = BuildXiph(dep_cfg)
	local SDL_LINK = BuildSDL(sdl_cfg, VariantBinSuffix(variant))
	local BLAKE3_LINK = BuildBLAKE3(dep_cfg, variant)
	local LIBWEBP_LINK = BuildLibWebPLosslessEncode(libwebp_cfg, variant)

	local link_cfg = logic_cfg:branch(
		BLAKE3_LINK, LIBWEBP_LINK, SDL_LINK, XIPH_LINK
	)
	local compile_cfg = link_cfg:branch(HATOYAMA_ENGINE.compile)

	local src
	src += HATOYAMA_ENGINE.src
	src += HATOYAMA.glob("engine/miniaudio/*.c*")
	src += HATOYAMA.glob("engine/windows/*.cpp")
	src += (HATOYAMA.glob("engine/sdl/*.cpp") - { "graphics_sdl.cpp$" })
	if (variant == MODERN) then
		src += HATOYAMA.glob("engine/sdl/graphics_sdl.cpp")
	end
	local obj = compile_cfg:cxx(src)

	if (variant == VINTAGE) then
		local vintage_cfg = compile_cfg:branch(ANALYSIS_RELAXED)
		local vintage_src = HATOYAMA.glob("engine/windows_vintage/DD*.CPP")
		vintage_src += HATOYAMA.glob("engine/windows_vintage/D2_Polygon.CPP")

		obj = (obj + vintage_cfg:cxx(vintage_src))
	end

	return link_cfg:branch({ lflags = "/SUBSYSTEM:windows", linputs = obj })
end
