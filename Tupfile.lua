tup.include("libs/tupblocks/Tuprules.lua")

---@class ConfigShape
SSG_COMPILE = {}
SSG_COMPILE.cflags = { "-I.", "-IGIAN07/", debug = "-DPBG_DEBUG" }
SSG_COMPILE.objdir = "ssg/"

SSG = sourcepath("./")

-- pbg code
SSG_SRC += SSG.glob("GIAN07/*.cpp")
SSG_SRC += SSG.glob("GIAN07/*.CPP")

-- Modern game code
LAYERS_SRC += SSG.glob("game/*.cpp")
LAYERS_SRC += SSG.glob("game/codecs/*.cpp")

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
