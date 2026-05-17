local BLAKE3 = sourcepath(tup.getcwd() .. "/BLAKE3/c/")

---Builds the BLAKE3 submodule.
---@param base_cfg Config
---@param generic integer Nonzero for a generic build without hand-written optimized versions for modern CPUs
function BuildBLAKE3(base_cfg, generic)
	local arch = tup.getconfig("TUP_ARCH")
	local platform = tup.getconfig("TUP_PLATFORM")

	---@type ConfigShape
	local link = { cflags = ("-I" .. BLAKE3.root) }

	---@type ConfigShape
	local compile = { objdir = "BLAKE3/", cflags = {} }
	if (platform == "win32") then
		-- Visual C++ has no SSE4.1 flag, and I don't trust the /arch:AVX option
		compile.cflags += "/DBLAKE3_NO_SSE41"

		if (generic ~= 0) then
			compile.cflags += {
				"/DBLAKE3_NO_SSE2", "/DBLAKE3_NO_AVX2", "/DBLAKE3_NO_AVX512",
			}
		end
	end
	local cfg = base_cfg:branch(compile)
	local src
	src += BLAKE3.join("blake3.c")
	src += BLAKE3.join("blake3_dispatch.c")
	src += BLAKE3.join("blake3_portable.c")

	local obj = cfg:cc(src)
	if (generic == 0) then
		if (platform == "win32") then
			-- Each optimized version must be built with the matching MSVC
			-- `/arch` flag. This is not only necessary for the compiler to
			-- actually emit the intended instructions, but also prevents newer
			-- instruction sets from accidentally appearing in older code. (For
			-- example, globally setting `/arch:AVX512` for all of these files
			-- would cause AVX-512 instructions to also appear in the AVX2
			-- version, breaking it on those CPUs.)
			-- That's why they recommend the ASM versions, but they're
			-- 64-bit-exclusive…
			local arch_cfg_1 = cfg:branch({ cflags = "/arch:SSE2" })
			local arch_cfg_2 = cfg:branch({ cflags = "/arch:AVX2" })
			local arch_cfg_3 = cfg:branch({ cflags = "/arch:AVX512" })
			obj = (
				obj +
				arch_cfg_1:cc(BLAKE3.join("blake3_sse2.c")) +
				arch_cfg_2:cc(BLAKE3.join("blake3_avx2.c")) +
				arch_cfg_3:cc(BLAKE3.join("blake3_avx512.c"))
			)
		else
			if (arch == "x86_64") then
				obj = (obj + cfg:cc(BLAKE3.glob("*unix.S")))
			end
		end
	end
	link.linputs = cfg:lib(obj, "BLAKE3")
	return link
end
