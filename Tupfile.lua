tup.include("libs/tupblocks/Tuprules.lua")

main_cfg = CONFIG:branch(tup.getconfig("BUILDTYPE"), {
	base = {
		cflags = (
			"/std:c++latest " ..
			"/DWIN32 " ..
			"/I. " ..
			"/EHsc " ..
			"/source-charset:utf-8 " ..
			"/execution-charset:shift-jis"
		),
		lflags = "obj/dinput.lib",
		objdir = "ssg/",
	},
	buildtypes = {
		debug = { cflags = "/DPBG_DEBUG" },
		release = { cflags = "/DNDEBUG" },
	}
})

main_src += tup.glob("game/*.cpp")
main_src += tup.glob("platform/windows/*.cpp")
main_src += tup.glob("DirectXUTYs/*.CPP")
main_src += tup.glob("DirectXUTYs/*.cpp")
main_src += tup.glob("GIAN07/*.cpp")
main_src += tup.glob("GIAN07/*.CPP")
main_src += "MAIN/MAIN.CPP"
exe(main_cfg, cxx(main_cfg, main_src), "GIAN07")
