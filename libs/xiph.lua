local LIBOGG = sourcepath(tup.getcwd() .. "/libogg/")
local LIBVORBIS = sourcepath(tup.getcwd() .. "/libvorbis/")

---Builds the submodules for the Xiph audio codecs.
---@param base_cfg Config
function BuildXiph(base_cfg)
	---@type ConfigShape
	local link = { cflags = string.format(
		"-I%sinclude -I%sinclude", LIBOGG.root, LIBVORBIS.root
	) }
	local libogg_cfg = base_cfg:branch(link, { objdir = "libogg/" })
	local libogg_src = LIBOGG.glob("src/*.c")

	local libvorbis_cfg = base_cfg:branch(link, { objdir = "libvorbis/" })
	local libvorbis_src = (LIBVORBIS.glob("lib/*.c") - {
		"barkmel.c$", "misc.c$", "psytune.c$", "tone.c$", "vorbisenc.c$"
	})

	link.linputs = (libogg_cfg:cc(libogg_src) + libvorbis_cfg:cc(libvorbis_src))
	return link
end
