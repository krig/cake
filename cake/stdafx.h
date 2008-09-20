#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

extern const char* g_compiler;
extern const char* g_compile_options[];
extern const char* g_link_options[];
extern const char* g_pch_options[];
extern char g_objdir[260];
extern const char* g_baseobjdir;
extern const char* g_bindir;
extern const char* g_libdir;
extern bool g_verbose_flag;
extern bool g_silent_flag;
extern bool g_rebuild_flag;
extern bool g_print_colors;
extern const char* g_target;
extern const char* HOME;

typedef std::vector<std::string> stringlist;

struct table
{
	typedef boost::shared_ptr<table> ptr;
	typedef std::map<std::string, std::string> vmap;
	typedef std::map<std::string, ptr> tmap;
	typedef stringlist svec;
	typedef std::vector<ptr> tvec; 

	vmap values;
	tmap children;
	svec strings;
	tvec tables;
};

#include "utility.h"

