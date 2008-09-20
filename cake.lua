--setup_scoria()

p = application("cake")
use_lua(p)


--[[
p:add_flags("-Icake/lua")
p:add_files(
	    "cake/lua/lapi.c",
	    "cake/lua/lcode.c",
	    "cake/lua/ldebug.c",
	    "cake/lua/ldo.c",
	    "cake/lua/ldump.c",
	    "cake/lua/lfunc.c",
	    "cake/lua/lgc.c",
	    "cake/lua/llex.c",
	    "cake/lua/lmem.c", 
	    "cake/lua/lobject.c",
	    "cake/lua/lopcodes.c",
	    "cake/lua/lparser.c",
	    "cake/lua/lstate.c",
	    "cake/lua/lstring.c",
	    "cake/lua/ltable.c",
	    "cake/lua/ltm.c", 
	    "cake/lua/lundump.c",
	    "cake/lua/lvm.c",
	    "cake/lua/lzio.c",
	    "cake/lua/lauxlib.c",
	    "cake/lua/lbaselib.c",
	    "cake/lua/ldblib.c",
	    "cake/lua/liolib.c",
	    "cake/lua/lmathlib.c",
	    "cake/lua/loslib.c",
	    "cake/lua/ltablib.c", 
	    "cake/lua/lstrlib.c",
	    "cake/lua/loadlib.c",
	    "cake/lua/linit.c")
--]]

p:add_files("cake/stdafx.h",
    	    "cake/*.cpp")

if target == "debug" then
   p:set_output_suffix("d")
end
