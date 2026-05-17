tup.include("libs/tupblocks/Tuprules.lua")

HATOYAMA = sourcepath(tup.getcwd() .. "/")

---@type ConfigShape
HATOYAMA_LINK = { cflags = { ("-I" .. HATOYAMA.root) } }

---@type ConfigShape
HATOYAMA_COMPILE = { objdir = "hatoyama/" }

HATOYAMA_SRC += HATOYAMA.glob("game/*.cpp")
HATOYAMA_SRC += HATOYAMA.glob("game/codecs/*.cpp")

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
