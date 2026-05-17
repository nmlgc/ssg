tup.import("TOOLCHAIN=gcc")
tup.include("libs/tupblocks/toolchain." .. TOOLCHAIN .. ".lua")
tup.include("libs/BLAKE3.lua")
