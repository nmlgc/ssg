local dep_cfg, logic_cfg = BuildHatoyamaLogic(HATOYAMA_CFLAGS)
local engine_cfg = BuildHatoyamaEngine(dep_cfg, logic_cfg)
local gian07_cfg = engine_cfg:branch(GIAN07_COMPILE)
local gian07_obj = gian07_cfg:cxx(GIAN07_OLD_SRC)

gian07_cfg:exe(gian07_obj, "GIAN07")
