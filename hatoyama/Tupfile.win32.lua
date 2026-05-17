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
