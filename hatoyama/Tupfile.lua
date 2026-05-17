tup.include("libs/tupblocks/Tuprules.lua")

tup.include(string.format("Tupfile.%s.lua", tup.getconfig("TUP_PLATFORM")))
