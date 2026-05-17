-- Icon
local ssg_ico = CONFIG:rc(SSG.join("GIAN07/GIAN07.rc"))

---@param variant integer
local function ssg(variant)
	local variant_cfg
	if (variant == MODERN) then
		variant_cfg = CONFIG:branch({
			cflags = {
				-- WebP only uses multithreading for effort levels 8 and 9,
				-- where it does significantly boost performance.
				"-DWEBP_USE_THREAD",
			},
			objdir = "modern/"
		})
	elseif (variant == VINTAGE) then
		variant_cfg = CONFIG:branch({
			cflags = {
				"/DWIN32_VINTAGE",
				"/D_WIN32_WINNT=0x0400", -- needed for libwebp
				"/D__WIN9X__",
				"/arch:IA32",
				"/Zc:threadSafeInit-",
			},
			lflags = flag_remove("/MANIFEST:.*"),
			objdir = "vintage/"
		}, {
			lflags = "/MANIFEST:NO" -- Saves 512 to 1024 bytes!
		})
	end

	local XIPH_LINK = BuildXiph(variant_cfg)
	local SDL_LINK = BuildSDL(variant_cfg, VariantBinSuffix(variant))
	local BLAKE3_LINK = BuildBLAKE3(variant_cfg, variant)
	local LIBWEBP_LINK = BuildLibWebPLosslessEncode(variant_cfg, variant)

	local modules_cfg = variant_cfg:branch(ANALYSIS)
	modules_cfg = modules_cfg:branch(modules_cfg:cxx_std_modules())

	-- The game
	-- --------

	local ssg_cfg = modules_cfg:branch(
		BLAKE3_LINK, LIBWEBP_LINK, XIPH_LINK, SDL_LINK, SSG_COMPILE, {
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
	ssg_obj = (ssg_obj + ssg_cfg:cxx(p_modern_src))

	if (variant == VINTAGE) then
		local COMPAT_LINK = Build9xcompat(variant_cfg)
		local p_vintage_cfg = ssg_cfg:branch(ANALYSIS_RELAXED)
		local p_vintage_src = SSG.glob("platform/windows_vintage/DD*.CPP")
		p_vintage_src += SSG.glob("platform/windows_vintage/D2_Polygon.CPP")

		ssg_obj = (ssg_obj + p_vintage_cfg:cxx(p_vintage_src))
		ssg_cfg = ssg_cfg:branch(COMPAT_LINK)
	end

	ssg_obj = (ssg_obj + ssg_ico)
	ssg_cfg:exe(ssg_obj, ("GIAN07" .. VariantBinSuffix(variant)))
end

ssg(MODERN)
ssg(VINTAGE)
