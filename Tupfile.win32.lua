-- Icon
local gian07_ico = CONFIG:rc(SSG.join("GIAN07/GIAN07.rc"))

local dep_cfgs = {}
local logic_cfgs = {}
local app_cfgs = {}
for _, variant in pairs({ MODERN, VINTAGE }) do
	local dep_cfg, logic_cfg = BuildHatoyamaLogic(variant, HATOYAMA_CFLAGS)
	local app_cfg = BuildHatoyamaApp(dep_cfg, logic_cfg, variant)
	dep_cfgs[variant] = dep_cfg
	logic_cfgs[variant] = logic_cfg
	app_cfgs[variant] = app_cfg
end

local api_link = BuildHatoyamaAPI(logic_cfgs[VINTAGE], LOGIC_VERSION)
local ssg_cfg = logic_cfgs[VINTAGE]:branch(SSG_COMPILE, api_link)
local ssg_obj = ssg_cfg:branch(ANALYSIS_RELAXED):cxx(SSG_SRC)
local ssg_link = ssg_cfg:dll(ssg_obj, ("ssg." .. LOGIC_VERSION))

---@param variant integer
local function gian07(variant)
	local dep_cfg = dep_cfgs[variant]
	local app_cfg = app_cfgs[variant]
	local engine_cfg = BuildHatoyamaEngine(dep_cfg, app_cfg, variant)

	local gian07_cfg = engine_cfg:branch(GIAN07_COMPILE, ssg_link)
	local gian07_obj = gian07_cfg:branch(ANALYSIS_RELAXED):cxx(GIAN07_OLD_SRC)

	if (variant == VINTAGE) then
		local COMPAT_LINK = Build9xcompat(dep_cfgs[variant])
		gian07_cfg = gian07_cfg:branch(COMPAT_LINK)
	end

	gian07_obj = (gian07_obj + gian07_ico)
	gian07_cfg:exe(gian07_obj, ("GIAN07" .. VariantBinSuffix(variant)))
end

gian07(MODERN)
gian07(VINTAGE)
BuildSSG_CLI(app_cfgs[MODERN], ssg_link)
