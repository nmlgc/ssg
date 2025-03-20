local LIBWEBP = sourcepath(tup.getcwd() .. "/libwebp_lossless/")

---Builds the WebP encoder from the libwebp_lossless submodule.
---@param base_cfg Config
---@param generic integer Nonzero for a generic build without hand-written optimized versions for modern CPUs
function BuildLibWebPLosslessEncode(base_cfg, generic)
	---@type ConfigShape
	local link = { cflags = string.format("-I%ssrc", LIBWEBP.root) }

	local src = {}
	local cfg = base_cfg:branch(link, {
		cflags = {
			("-I" .. LIBWEBP.root),
			"-DWEBP_DISABLE_STATS",
			"-DWEBP_NEAR_LOSSLESS=0", -- We want actual lossless, not "near"
			"-DWEBP_REDUCE_SIZE",

			-- WebP only uses multithreading for effort levels 8 and 9, where
			-- it does significantly boost performance.
			"-DWEBP_USE_THREAD",
		},
		objdir = "libwebp/",
	})
	if (generic ~= 0) then
		src.extra_inputs += Header((cfg.vars.objdir .. "src/webp/config.h"), {
			WEBP_HAVE_SSE2 = false,
			WEBP_HAVE_SSE41 = false,
			WEBP_HAVE_AVX2 = false,
		})
		cfg = cfg:branch({
			cflags = { ("-I" .. cfg.vars.objdir), "-DHAVE_CONFIG_H" },
		})
	end

	src += LIBWEBP.glob("src/dsp/alpha_processing*.c")
	src += LIBWEBP.glob("src/dsp/cpu.c")
	src += LIBWEBP.glob("src/dsp/enc*.c")
	src += LIBWEBP.glob("src/dsp/lossless*.c")
	src += (LIBWEBP.glob("src/enc/*.c") - {
		"picture_psnr_enc.c$", "near_lossless_enc.c$"
	})
	src += (LIBWEBP.glob("src/utils/*.c") - {
		"bit_reader_utils.c$",
		"filters_utils.c$",
		"huffman_utils.c$",
		"rescaler_utils.c$",
		"quant_levels_dec_utils.c",
		"random_utils.c$",
	})
	local _ = TableDrain(src, { "_neon.c$", "_mips.*.c$", "_msa.c$" })
	local src_x86ext
	src_x86ext += TableDrain(src, { "_sse2.c$" })
	src_x86ext += TableDrain(src, { "_sse41.c$" })
	src_x86ext += TableDrain(src, { "_avx2.c$" })
	local obj = cfg:cc(src)
	if (generic == 0) then
		-- Since Visual Studio sadly has no `/arch:SSE41` flag, we need to
		-- limit the optimizer to SSE2 levels.
		obj = (obj + cfg:branch({ cflags = "/arch:SSE2" }):cc(src_x86ext))
	end

	link.linputs = cfg:lib(obj, "libwebp_lossless")
	return link
end
