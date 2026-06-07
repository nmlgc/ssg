tup.include("hatoyama/Tupfile.lua")

API_VERSION = "1"
LOGIC_VERSION = "v1.005"

HATOYAMA_LINK.cflags += { "-I.", "-IGIAN07/" }

---@type ConfigShape
SSG_COMPILE = { objdir = "ssg/" }

---@type ConfigShape
GIAN07_COMPILE = {}
GIAN07_COMPILE.cflags = { debug = "-DPBG_DEBUG" }
GIAN07_COMPILE.objdir = "ssg/"

SSG = sourcepath("./")

PLATFORM_CONSTANTS = EnvHeader(SSG.join("obj/platform_constants.h"), {
	"APP_ID", "PATH_SKELETON"
})

-- Logic layer
SSG_SRC += SSG.glob("ssg/*.cpp")
SSG_SRC += SSG.glob("ssg/internal/*.cpp")

---@param logic_cfg Config
---@param api_link ConfigShape
function BuildSSG_LogicObjs(logic_cfg, api_link)
	local compile_cfg = logic_cfg:branch(SSG_COMPILE, api_link)

	local lines = { "// Validate that public headers are valid C" }
	for _, h_fn in pairs(tup.glob("ssg/*.h")) do
		h_fn = h_fn:gsub("([/\\]+)", "/")
		table.insert(lines, string.format('#include "%s"', h_fn))
	end
	local validate_cfg = CONFIG:branch({ cflags = "-I." })
	validate_cfg.vars.objdir = compile_cfg.vars.objdir
	local validate_src = File(
		(compile_cfg.vars.objdir .. "validate_c_headers.c"), lines
	)
	local validate_obj = validate_cfg:cc(validate_src)

	local validate_cinputs = {}
	for buildtype, objs in pairs(validate_obj) do
		validate_cinputs[buildtype] = { extra_inputs = objs[1] }
	end
	local ret = compile_cfg:branch({ cinputs = validate_cinputs }):cxx(SSG_SRC)
	return compile_cfg, ret
end

---@param app_cfg Config
---@param logic_link ConfigShape
function BuildSSG_CLI(app_cfg, logic_link)
	local cfg = app_cfg:branch(logic_link)
	if (tup.getconfig("TUP_PLATFORM") == "win32") then
		-- Critically important for redirecting output!
		cfg = cfg:branch({ lflags = "/SUBSYSTEM:console" })
	end
	local obj = cfg:cxx(SSG.glob("cli/*.cpp"))
	cfg:exe(obj, "ssg_cli")
end

-- pbg code
GIAN07_OLD_SRC += SSG.glob("GIAN07/*.cpp")
GIAN07_OLD_SRC += SSG.glob("GIAN07/*.CPP")
GIAN07_OLD_SRC += "MAIN/main_sdl.cpp"
GIAN07_OLD_SRC.extra_inputs += PLATFORM_CONSTANTS

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
