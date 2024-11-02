local NINEXCOMPAT = sourcepath(tup.getcwd() .. "/9xcompat/")

---Builds the 9xcompat submodule.
---@param base_cfg Config
---@return ConfigShape
function Build9xcompat(base_cfg)
	local cfg = base_cfg:branch({
		cflags = { "/std:c++latest", "/GS-" },
		lflags = { "kernel32.lib" },
	})

	-- 9xcompat doesn't make sense for DLL CRTs because it can't influence
	-- their imports.
	for index_or_buildtype, flag_or_vars in pairs(cfg.vars.cflags) do
		if (type(flag_or_vars) == "table") then
			for _, flag in pairs(flag_or_vars) do
				if flag:match("^/MD") then
					cfg.vars.cflags[index_or_buildtype] = nil
					cfg.buildtypes[index_or_buildtype] = nil
					break
				end
			end
		else
			if flag_or_vars:match("^/MD") then
				return {}
			end
		end
	end

	local compat_obj = cfg:cxx(NINEXCOMPAT.join("9xcompat.cpp"))
	local aliases_obj = tup.rule(
		NINEXCOMPAT.join("aliases.asm"),
		'ml /nologo /c /Fo "%o" "%f"',
		(cfg.vars.objdir .. "9xcompat_%B.obj")
	)

	-- Only add `aliases.obj` to the types we actually build.
	-- TODO: Won't be necessary once tupblocks has proper ASM support.
	local link = { linputs = compat_obj }
	for buildtype, fn in pairs(compat_obj) do
		link.linputs[buildtype] += aliases_obj[1]
	end
	return link
end
