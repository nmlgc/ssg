tup.include("libs/tupblocks/Tuprules.lua")

HATOYAMA = sourcepath(tup.getcwd() .. "/")

---@type ConfigShape
HATOYAMA_LINK = { cflags = { ("-I" .. HATOYAMA.root) } }

---@param name string
---@return { compile: ConfigShape, src: string[] }
local function hatoyama_part(name)
	return {
		compile = { objdir = (HATOYAMA.root .. name .. "/") },
		src = HATOYAMA.glob(name .. "/*.cpp"),
	}
end

HATOYAMA_LOGIC = hatoyama_part("logic")
HATOYAMA_API = hatoyama_part("api")
HATOYAMA_ENGINE = hatoyama_part("engine")

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))

---@param logic_cfg Config
---@param logic_version string
---@return ConfigShape
function BuildHatoyamaAPI(logic_cfg, logic_version)
	---@type ConfigShape
	local link = { cflags = { "-DHATOYAMA_API_BUILD" } }
	local cfg = logic_cfg:branch(link, HATOYAMA_API.compile)

	local src = HATOYAMA_API.src
	src.extra_inputs += Header((cfg.vars.objdir .. "logic_version.h"),
		{ LOGIC_VERSION = logic_version }
	)
	link.linputs = cfg:branch({ cflags = ("-I" .. cfg.vars.objdir) }):cxx(src)
	return link
end
