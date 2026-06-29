HATOYAMA_LOGIC.compile.cflags += { '-fPIC', '-fvisibility=hidden' }
SSG_COMPILE.cflags += { '-fPIC', '-fvisibility=hidden' }

HATOYAMA_API.compile.cflags += { '-fPIC', '-fvisibility=hidden' }

-- Nothing in this project actually uses libstdc++ symbols directly here.
SSG_COMPILE.lflags += { '-nostdlib++' }

local dep_cfg, logic_cfg = BuildHatoyamaLogic(HATOYAMA_CFLAGS)
local api_link = BuildHatoyamaAPI(logic_cfg, LOGIC_VERSION)

local ssg_cfg = logic_cfg:branch(SSG_COMPILE, api_link)
local ssg_obj = ssg_cfg:cxx(SSG_SRC)
local ssg_link = ssg_cfg:dll(ssg_obj, "ssg", API_VERSION, LOGIC_VERSION)

local app_cfg = BuildHatoyamaApp(logic_cfg)
local engine_cfg = BuildHatoyamaEngine(dep_cfg, app_cfg)
local gian07_cfg = engine_cfg:branch(GIAN07_COMPILE, ssg_link)
local gian07_obj = gian07_cfg:cxx(GIAN07_OLD_SRC)

gian07_cfg:exe(gian07_obj, "GIAN07")

BuildSSG_CLI(app_cfg, ssg_link)