-- Icon
local gian07_ico = CONFIG:rc(SSG.join("GIAN07/GIAN07.rc"))

---@param variant integer
local function gian07(variant)
	local dep_cfg, logic_cfg = BuildHatoyamaLogic(variant, HATOYAMA_CFLAGS)
	local api_link = BuildHatoyamaAPI(logic_cfg, LOGIC_VERSION)

	local ssg_cfg = logic_cfg:branch(SSG_COMPILE, api_link)
	local ssg_obj = ssg_cfg:branch(ANALYSIS_RELAXED):cxx(SSG_SRC)
	local ssg_link = ssg_cfg:dll(
		ssg_obj, ("ssg." .. LOGIC_VERSION .. VariantBinSuffix(variant))
	)

	local engine_cfg = BuildHatoyamaEngine(dep_cfg, logic_cfg, variant)

	local gian07_cfg = engine_cfg:branch(GIAN07_COMPILE, ssg_link)
	local gian07_obj = gian07_cfg:branch(ANALYSIS_RELAXED):cxx(GIAN07_OLD_SRC)

	if (variant == VINTAGE) then
		local COMPAT_LINK = Build9xcompat(dep_cfg)
		gian07_cfg = gian07_cfg:branch(COMPAT_LINK)
	end

	gian07_obj = (gian07_obj + gian07_ico)
	gian07_cfg:exe(gian07_obj, ("GIAN07" .. VariantBinSuffix(variant)))
end

gian07(MODERN)
gian07(VINTAGE)
