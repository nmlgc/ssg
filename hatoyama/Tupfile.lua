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
HATOYAMA_ENGINE = hatoyama_part("engine")

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
