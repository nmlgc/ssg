-- Icon
local ssg_ico = CONFIG:rc(SSG.join("GIAN07/GIAN07.rc"))

---@param variant integer
local function ssg(variant)
	local dep_cfg, logic_cfg = BuildHatoyamaLogic(variant, HATOYAMA_CFLAGS)
	local engine_cfg = BuildHatoyamaEngine(dep_cfg, logic_cfg, variant)

	local ssg_cfg = engine_cfg:branch(SSG_COMPILE)
	local ssg_obj = ssg_cfg:branch(ANALYSIS_RELAXED):cxx(SSG_SRC)

	if (variant == VINTAGE) then
		local COMPAT_LINK = Build9xcompat(dep_cfg)
		ssg_cfg = ssg_cfg:branch(COMPAT_LINK)
	end

	ssg_obj = (ssg_obj + ssg_ico)
	ssg_cfg:exe(ssg_obj, ("GIAN07" .. VariantBinSuffix(variant)))
end

ssg(MODERN)
ssg(VINTAGE)
