tup.include("hatoyama/Tupfile.lua")

HATOYAMA_CFLAGS = { "-I.", "-IGIAN07/" }

---@type ConfigShape
SSG_COMPILE = {}
SSG_COMPILE.cflags = { debug = "-DPBG_DEBUG" }
SSG_COMPILE.objdir = "ssg/"

SSG = sourcepath("./")

PLATFORM_CONSTANTS = EnvHeader(SSG.join("obj/platform_constants.h"), {
	"APP_ID", "PATH_SKELETON"
})

-- pbg code
SSG_SRC += SSG.glob("GIAN07/*.cpp")
SSG_SRC += SSG.glob("GIAN07/*.CPP")
SSG_SRC += "MAIN/main_sdl.cpp"
SSG_SRC.extra_inputs += PLATFORM_CONSTANTS

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
