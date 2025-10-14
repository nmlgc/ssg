local SDL = sourcepath(tup.getcwd() .. "/SDL3/")

---Builds the SDL3 submodule for Windows.
---@param base_cfg Config
---@param bin_suffix string
function BuildSDL(base_cfg, bin_suffix)
	local name = "SDL3"
	local modern = not MatchesAny(function(var)
		return string.match(var, "[/-]D__WIN9X__")
	end, base_cfg.vars.cflags)
	local compile = {
		cflags = {
			"/DDLL_EXPORT",
			("-I" .. SDL.join("include/build_config/")),
			("-I" .. SDL.join("src/")),
		},
		lflags = {
			"advapi32.lib",
			"imm32.lib",
			"gdi32.lib",
			"kernel32.lib",
			"ole32.lib",
			"oleaut32.lib",
			"setupapi.lib",
			"user32.lib",
			"uuid.lib",
			"version.lib",
			"winmm.lib",
		},
		objdir = (name .. "/"),
	}
	if not modern then
		compile.cflags += "/GS-"
		compile.lflags += "/NODEFAULTLIB"
	end

	---@type ConfigShape
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
	if modern then
		src += (SDL.glob("src/audio/wasapi/*.c") - { "winrt.cpp$" })
	end
	src += SDL.glob("src/camera/*.c")
	src += SDL.glob("src/camera/dummy/*.c")
	if modern then
		src += SDL.glob("src/camera/mediafoundation/*.c")
	end
	src += SDL.glob("src/core/*.c")
	src += SDL.glob("src/core/windows/*.c")
	if modern then
		src += SDL.join("src/core/windows/SDL_gameinput.cpp")
	end
	src += SDL.glob("src/cpuinfo/*.c")
	src += SDL.glob("src/dialog/*.c")
	src += SDL.glob("src/dialog/windows/*.c")
	if modern then
		src += SDL.glob("src/dynapi/*.c")
	end
	src += (SDL.glob("src/events/*.c") - {
		"imKStoUCS.c$", "SDL_keysym_to_scancode.c$", "SDL_scancode_tables.c$",
	})
	src += SDL.glob("src/filesystem/*.c")
	src += SDL.glob("src/filesystem/windows/*.c")
	src += SDL.glob("src/gpu/*.c")
	if modern then
		src += SDL.glob("src/gpu/d3d12/*.c")
		src += SDL.glob("src/gpu/vulkan/*.c")
	end
	src += SDL.glob("src/haptic/*.c")
	src += SDL.glob("src/haptic/hidapi/*.c")
	src += SDL.glob("src/haptic/windows/*.c")
	src += SDL.glob("src/hidapi/*.c")
	src += SDL.glob("src/io/*.c")
	src += SDL.glob("src/io/generic/*.c")
	if modern then
		src += SDL.glob("src/io/windows/*.c")
	end
	src += SDL.glob("src/joystick/*.c")
	if modern then
		src += SDL.glob("src/joystick/gdk/*.cpp")
	end
	src += SDL.glob("src/joystick/hidapi/*.c")
	src += SDL.glob("src/joystick/virtual/*.c")
	src += (SDL.glob("src/joystick/windows/*.c") - {
		"SDL_rawinputjoystick.c$", "SDL_windows_gaming_input.c$",
	})
	if modern then
		src += SDL.join("src/joystick/windows/SDL_rawinputjoystick.c")
		src += SDL.join("src/joystick/windows/SDL_windows_gaming_input.c")
	end
	src += SDL.glob("src/libm/*.c")
	src += SDL.glob("src/loadso/windows/*.c")
	src += SDL.glob("src/locale/*.c")
	src += SDL.glob("src/locale/windows/*.c")
	src += SDL.glob("src/main/*.c")
	src += SDL.glob("src/main/generic/*.c")
	src += SDL.glob("src/main/windows/*.c")
	src += SDL.glob("src/misc/*.c")
	src += SDL.glob("src/misc/windows/*.c")
	src += SDL.glob("src/power/*.c")
	src += SDL.glob("src/power/windows/*.c")
	src += SDL.glob("src/process/*.c")
	src += SDL.glob("src/process/windows/*.c")
	src += SDL.glob("src/render/*.c")
	src += SDL.glob("src/render/direct3d/*.c")
	if modern then
		src += (SDL.glob("src/render/direct3d11/*.c") - { "winrt.cpp$" })
		src += (SDL.glob("src/render/direct3d12/*.c") - { "winrt.cpp$" })
		src += SDL.glob("src/render/gpu/*.c")
		src += SDL.glob("src/render/opengl/*.c")
		src += SDL.glob("src/render/opengles2/*.c")
		src += SDL.glob("src/render/vulkan/*.c")
	else
		-- Even the vintage build should still have SDL_RenderGetD3D11Device()
		-- and SDL_RenderGetD3D12Device(), which are expected to always exist
		-- in Windows builds of SDL 2.
		src += SDL.join("src/render/direct3d11/SDL_render_d3d11.c")
		src += SDL.join("src/render/direct3d12/SDL_render_d3d12.c")
	end
	src += SDL.glob("src/render/software/*.c")
	src += SDL.glob("src/sensor/*.c")
	if modern then
		src += SDL.glob("src/sensor/windows/*.c")
	else
		src += SDL.glob("src/sensor/dummy/*.c")
	end
	src += (SDL.glob("src/stdlib/*.c") - { "SDL_mem.+.c$", "SDL_mslibc.c$" })
	src += SDL.glob("src/storage/*.c")
	src += SDL.glob("src/storage/generic/*.c")
	src += SDL.glob("src/thread/*.c")
	src += SDL.glob("src/thread/generic/SDL_sysrwlock.c")
	src += SDL.join("src/thread/generic/SDL_syscond.c")
	src += SDL.glob("src/thread/windows/*.c")
	src += SDL.glob("src/time/*.c")
	src += SDL.glob("src/time/windows/*.c")
	src += SDL.glob("src/timer/*.c")
	src += SDL.glob("src/timer/windows/*.c")
	src += SDL.glob("src/tray/*.c")
	src += SDL.glob("src/tray/windows/*.c")
	src += SDL.glob("src/video/*.c")
	src += (SDL.glob("src/video/*.c") - { "SDL_egl.c$", "SDL_vulkan_utils.c$" })
	if modern then
		src += SDL.join("src/video/SDL_egl.c")
		src += SDL.join("src/video/SDL_vulkan_utils.c")
	end
	src += SDL.glob("src/video/dummy/*.c")
	src += (SDL.glob("src/video/offscreen/*.c") - {
		"opengles.c$", "vulkan.c$",
	})
	if modern then
		src += SDL.glob("src/video/offscreen/*.c")
	end
	src += (SDL.glob("src/video/windows/*.c") - { "SDL_windowsvulkan.c$" })
	if modern then
		src += SDL.join("src/video/windows/SDL_windowsvulkan.c")
	end
	src += SDL.glob("src/video/windows/*.cpp")
	src += SDL.glob("src/video/yuv2rgb/*.c")
	local version_rc = SDL.join("src/core/windows/version.rc")

	local cfg = base_cfg:branch(compile, link)
	local mslibc_cfg = cfg:branch(
		{ cflags = { release = flag_remove("/GL") } }
	)
	local mslibc_src
	mslibc_src += SDL.glob("src/stdlib/SDL_mem*.c")
	mslibc_src += SDL.join("src/stdlib/SDL_mslibc.c")
	local obj = (
		cfg:cc(src) +
		mslibc_cfg:cc(mslibc_src) +
		cfg:rc(version_rc)
	)

	link.cflags += ("-D" .. name .. "=1")
	link.linputs = cfg:dll(obj, (name .. bin_suffix))
	return link
end
