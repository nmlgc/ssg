local dep_cfg, logic_cfg = BuildHatoyamaLogic(HATOYAMA_CFLAGS)
local engine_cfg = BuildHatoyamaEngine(dep_cfg, logic_cfg)
local ssg_cfg = engine_cfg:branch(SSG_COMPILE)
local ssg_obj = ssg_cfg:cxx(SSG_SRC)

ssg_cfg:exe(ssg_obj, "GIAN07")
