p = program("cake")
p:add_pkgdepend("lua5.1")

p:add_files("cake/stdafx.h",
    	    "cake/*.cpp")

if target == "debug" then
   p:set_output_suffix("d")
end
