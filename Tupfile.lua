tup.include("hatoyama/Tupfile.lua")

HATOYAMA_CFLAGS = { "-I.", "-IGIAN07/" }

---@type ConfigShape
GIAN07_COMPILE = {}
GIAN07_COMPILE.cflags = { debug = "-DPBG_DEBUG" }
GIAN07_COMPILE.objdir = "ssg/"

SSG = sourcepath("./")

PLATFORM_CONSTANTS = EnvHeader(SSG.join("obj/platform_constants.h"), {
	"APP_ID", "PATH_SKELETON"
})

-- pbg code
GIAN07_OLD_SRC += SSG.glob("GIAN07/*.cpp")
GIAN07_OLD_SRC += SSG.glob("GIAN07/*.CPP")
GIAN07_OLD_SRC += SSG.glob("ssg/internal/*.cpp")
GIAN07_OLD_SRC += "MAIN/main_sdl.cpp"
GIAN07_OLD_SRC.extra_inputs += PLATFORM_CONSTANTS

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
