local SDL_PATHS = {
	[2] = sourcepath(tup.getcwd() .. "/SDL2/"),
	[3] = sourcepath(tup.getcwd() .. "/SDL3/"),
}

---Builds the SDL submodule for Windows.
---@param base_cfg Config
---@param version 2 | 3
---@param bin_suffix string
function BuildSDL(base_cfg, version, bin_suffix)
	local name = ((version == 3) and "SDL3" or "SDL2")
	local SDL = SDL_PATHS[version]
	local modern = not MatchesAny(function(var)
		return string.match(var, "[/-]D__WIN9X__")
	end, base_cfg.vars.cflags)
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
		},
		objdir = (name .. "/"),
	}
	if ((version == 2) and modern) then
		compile.lflags.debug = "ucrtd.lib"
		compile.lflags.release = "ucrt.lib"
	end

	---@type ConfigShape
	local link = {
		cflags = ("-I" .. SDL.join("include/")),
		lflags = { "shell32.lib" }, -- for SDL_main()'s CommandLineToArgvW()
	}
	if (version == 3) then
		compile.cflags += ("-I" .. SDL.join("include/build_config/"))
		compile.cflags += ("-I" .. SDL.join("src/"))
	end

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
	src += SDL.glob("src/core/windows/*.c")
	src += SDL.glob("src/cpuinfo/*.c")
	src += SDL.glob("src/dynapi/*.c")
	src += (SDL.glob("src/events/*.c") - {
		"imKStoUCS.c$", "SDL_keysym_to_scancode.c$", "SDL_scancode_tables.c$",
	})
	src += SDL.glob("src/filesystem/windows/*.c")
	src += SDL.glob("src/haptic/*.c")
	if (version == 3) then
		src += SDL.glob("src/haptic/hidapi/*.c")
	end
	src += SDL.glob("src/haptic/windows/*.c")
	src += SDL.glob("src/hidapi/*.c")
	src += SDL.glob("src/joystick/*.c")
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
	src += SDL.glob("src/misc/*.c")
	src += SDL.glob("src/misc/windows/*.c")
	src += SDL.glob("src/power/*.c")
	src += SDL.glob("src/power/windows/*.c")
	src += SDL.glob("src/render/*.c")
	src += SDL.glob("src/render/direct3d/*.c")
	if modern then
		src += (SDL.glob("src/render/direct3d11/*.c") - { "winrt.cpp$" })
		src += (SDL.glob("src/render/direct3d12/*.c") - { "winrt.cpp$" })
		src += SDL.glob("src/render/opengl/*.c")
		src += SDL.glob("src/render/opengles2/*.c")
	else
		-- Even the vintage build should still have SDL_RenderGetD3D11Device()
		-- and SDL_RenderGetD3D12Device(), which are expected to always exist
		-- in Windows builds of SDL 2.
		src += SDL.join("src/render/direct3d11/SDL_render_d3d11.c")
		src += SDL.join("src/render/direct3d12/SDL_render_d3d12.c")
	end
 	src += SDL.glob("src/render/software/*.c")
	src += SDL.glob("src/sensor/*.c")
	src += SDL.glob("src/sensor/windows/*.c")
	src += (SDL.glob("src/stdlib/*.c") - { "SDL_mslibc.c$" })
	src += SDL.glob("src/thread/*.c")
	src += SDL.glob("src/thread/windows/*.c")
	src += SDL.glob("src/timer/*.c")
	src += SDL.glob("src/timer/windows/*.c")
	src += SDL.glob("src/video/*.c")
	src += (SDL.glob("src/video/*.c") - { "SDL_vulkan_utils.c$" })
	if modern then
		src += SDL.join("src/video/SDL_vulkan_utils.c")
	end
	src += SDL.glob("src/video/dummy/*.c")
	src += (SDL.glob("src/video/windows/*.c") - { "SDL_windowsvulkan.c$" })
	if modern then
		src += SDL.join("src/video/windows/SDL_windowsvulkan.c")
	end
	src += SDL.glob("src/video/yuv2rgb/*.c")
	src += SDL.join("src/thread/generic/SDL_syscond.c")
	if (version == 3) then
		src += SDL.glob("src/camera/*.c")
		src += SDL.glob("src/camera/dummy/*.c")
		src += SDL.glob("src/camera/mediafoundation/*.c")
		src += SDL.glob("src/core/*.c")
		src += SDL.glob("src/core/windows/*.cpp")
		src += SDL.glob("src/dialog/*.c")
		src += SDL.glob("src/dialog/windows/*.c")
		src += SDL.glob("src/filesystem/*.c")
		src += SDL.glob("src/gpu/*.c")
		src += SDL.glob("src/gpu/d3d12/*.c")
		src += SDL.glob("src/gpu/vulkan/*.c")
		src += SDL.glob("src/io/*.c")
		src += SDL.glob("src/io/generic/*.c")
		src += SDL.glob("src/io/windows/*.c")
		src += SDL.glob("src/joystick/gdk/*.cpp")
		src += SDL.glob("src/main/*.c")
		src += SDL.glob("src/main/generic/*.c")
		src += SDL.glob("src/main/windows/*.c")
		src += SDL.glob("src/process/*.c")
		src += SDL.glob("src/process/windows/*.c")
		src += SDL.glob("src/render/gpu/*.c")
		src += SDL.glob("src/render/vulkan/*.c")
		src += SDL.glob("src/storage/*.c")
		src += SDL.glob("src/storage/generic/*.c")
		src += SDL.glob("src/thread/generic/SDL_sysrwlock.c")
		src += SDL.glob("src/time/*.c")
		src += SDL.glob("src/time/windows/*.c")
		src += SDL.glob("src/tray/*.c")
		src += SDL.glob("src/tray/windows/*.c")
		src += SDL.glob("src/video/offscreen/*.c")
		src += SDL.glob("src/video/windows/*.cpp")
	else
		src += SDL.glob("src/file/*.c")
		src += SDL.glob("src/audio/winmm/*.c")
	end
	local version_rc = ((version == 3) and
		SDL.join("src/core/windows/version.rc") or
		SDL.join("src/main/windows/version.rc")
	)

	local cfg = base_cfg:branch(compile, link)
	local mslibc_cfg = cfg:branch(
		{ cflags = { release = flag_remove("/GL") } }
	)
	local obj = (
		cfg:cc(src) +
		mslibc_cfg:cc(SDL.join("src/stdlib/SDL_mslibc.c")) +
		cfg:rc(version_rc)
	)

	link.cflags += ("-D" .. name .. "=1")
	link.linputs = cfg:dll(obj, (name .. bin_suffix))
	if (version == 2) then
		local src_winmain = SDL.glob("src/main/windows/*.c")
		link.linputs = (link.linputs + cfg:cc(src_winmain))
	end
	return link
end
