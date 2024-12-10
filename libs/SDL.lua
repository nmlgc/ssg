local SDL = sourcepath(tup.getcwd() .. "/SDL/")

---Builds the SDL submodule for Windows.
---@param base_cfg Config
function BuildSDL(base_cfg)
	local compile = {
		cflags = "/DDLL_EXPORT",
		lflags = {
			"advapi32.lib",
			"imm32.lib",
			"gdi32.lib",
			"kernel32.lib",
			"ole32.lib",
			"oleaut32.lib",
			"setupapi.lib",
			"user32.lib",
			"version.lib",
			"winmm.lib",
			debug = "ucrtd.lib",
			release = "ucrt.lib",
		},
		objdir = "SDL/",
	}

	---@class ConfigShape
	local link = {
		cflags = ("-I" .. SDL.join("include/")),
		lflags = { "shell32.lib" }, -- for SDL_main()'s CommandLineToArgvW()
	}

	local src
	src += SDL.glob("src/*.c")
	src += SDL.glob("src/atomic/*.c")
	src += (SDL.glob("src/audio/*.c") - { "SDL_audiodev.c$" })
	src += SDL.glob("src/audio/directsound/*.c")
	src += SDL.glob("src/audio/disk/*.c")
	src += SDL.glob("src/audio/dummy/*.c")
	src += SDL.glob("src/audio/winmm/*.c")
	src += (SDL.glob("src/audio/wasapi/*.c") - { "winrt.cpp$" })
	src += SDL.glob("src/core/windows/*.c")
	src += SDL.glob("src/cpuinfo/*.c")
	src += SDL.glob("src/dynapi/*.c")
	src += (SDL.glob("src/events/*.c") - {
		"imKStoUCS.c$", "SDL_keysym_to_scancode.c$", "SDL_scancode_tables.c$",
	})
	src += SDL.glob("src/file/*.c")
	src += SDL.glob("src/filesystem/windows/*.c")
	src += SDL.glob("src/haptic/*.c")
	src += SDL.glob("src/haptic/windows/*.c")
	src += SDL.glob("src/hidapi/*.c")
	src += SDL.glob("src/joystick/*.c")
	src += SDL.glob("src/joystick/hidapi/*.c")
	src += SDL.glob("src/joystick/virtual/*.c")
	src += SDL.glob("src/joystick/windows/*.c")
	src += SDL.glob("src/libm/*.c")
	src += SDL.glob("src/loadso/windows/*.c")
	src += SDL.glob("src/locale/*.c")
	src += SDL.glob("src/locale/windows/*.c")
	src += SDL.glob("src/misc/*.c")
	src += SDL.glob("src/misc/windows/*.c")
	src += SDL.glob("src/power/*.c")
	src += SDL.glob("src/power/windows/*.c")
	src += SDL.glob("src/render/*.c")
	src += SDL.glob("src/render/direct3d/*.c")
	src += (SDL.glob("src/render/direct3d11/*.c") - { "winrt.cpp$" })
	src += (SDL.glob("src/render/direct3d12/*.c") - { "winrt.cpp$" })
	src += SDL.glob("src/render/opengl/*.c")
	src += SDL.glob("src/render/opengles2/*.c")
	src += SDL.glob("src/render/software/*.c")
	src += SDL.glob("src/sensor/*.c")
	src += SDL.glob("src/sensor/windows/*.c")
	src += (SDL.glob("src/stdlib/*.c") - { "SDL_mslibc.c$" })
	src += SDL.glob("src/thread/*.c")
	src += SDL.glob("src/thread/windows/*.c")
	src += SDL.glob("src/timer/*.c")
	src += SDL.glob("src/timer/windows/*.c")
	src += SDL.glob("src/video/*.c")
	src += SDL.glob("src/video/dummy/*.c")
	src += SDL.glob("src/video/windows/*.c")
	src += SDL.glob("src/video/yuv2rgb/*.c")
	src += SDL.join("src/thread/generic/SDL_syscond.c")
	local src_winmain = SDL.glob("src/main/windows/*.c")

	local cfg = base_cfg:branch(compile, link)
	local mslibc_cfg = cfg:branch(
		{ cflags = { release = flag_remove("/GL") } }
	)
	local obj = (
		cc(cfg, src) +
		cc(mslibc_cfg, SDL.join("src/stdlib/SDL_mslibc.c")) +
		rc(cfg, SDL.join("src/main/windows/version.rc"))
	)
	link.linputs = (dll(cfg, obj, "SDL2") + cc(cfg, src_winmain))
	return link
end
