local PRINTF = sourcepath(tup.getcwd() .. "/printf/src/")

---Builds the printf submodule.
---@param base_cfg Config
function BuildPrintf(base_cfg)
	local link = { cflags = {
		string.format("-I%s", (PRINTF.root .. "../../")),
		string.format("-I%s", PRINTF.root),
		"-DPRINTF_INCLUDE_CONFIG_H=1",
	} }

	---@type ConfigShape
	local compile = { objdir = "printf/" }

	local cfg = base_cfg:branch(link, compile)
	link.linputs = cfg:cc(PRINTF.glob("printf/*.c"))
	return link
end
