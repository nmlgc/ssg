local hatoyama_cfg = BuildHatoyama(HATOYAMA_CFLAGS)
local ssg_cfg = hatoyama_cfg:branch(SSG_COMPILE)
local ssg_obj = ssg_cfg:cxx(SSG_SRC)

ssg_cfg:exe(ssg_obj, "GIAN07")
