BASE = {
	cflags = (
		"/std:c++latest " ..
		"/DWIN32 " ..
		"/I. " ..
		"/EHsc " ..
		"/source-charset:utf-8 " ..
		"/execution-charset:shift-jis "
	),
	lflags = "obj/dinput.lib",
	objdir = "obj/",
	bindir = "bin/",
}

if (tup.getconfig("DEBUG") != "n") then
	CONFIGS.debug.cflags = (CONFIGS.debug.cflags .. " /DPBG_DEBUG")
end

main_src += tup.glob("platform/windows/*.cpp")
main_src += tup.glob("DirectXUTYs/*.CPP")
main_src += tup.glob("DirectXUTYs/*.cpp")
main_src += tup.glob("GIAN07/*.cpp")
main_src += tup.glob("GIAN07/*.CPP")
main_src += "MAIN/MAIN.CPP"
exe(cxx(main_src, "", ""), "", "GIAN07")
