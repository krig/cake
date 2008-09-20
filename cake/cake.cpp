#include "stdafx.h"
#include <argp.h>
#include "build.h"
#include "file.h"
#include "command.h"
#include "arglist.h"
#include "project.h"

using namespace std;

const char *argp_program_version = "cake 0.1";
const char *argp_program_bug_address = "<kegie@antispam.you.figure.it.out.com>";
static char cake_doc[] = "Let Them Eat Cake. (don't let them eat make).";
static char args_doc[] = "TARGET";

const char* g_compiler = "gcc";
const char* g_pch_options[] = {"-MMD", "-I.", "-fpch-deps", ""/*terminator*/};
const char* g_compile_options[] = {"-c", "-MMD", "-I.", "-fpch-deps", ""/*terminator*/};
const char* g_link_options[] = {""/*terminator*/};
const char* g_baseobjdir = "obj/";
char g_objdir[260] = "obj/";
const char* g_bindir = "bin/";
const char* g_libdir = "lib/";
const char* g_makefile = "cake.lua";
bool g_verbose_flag = false;
bool g_silent_flag = false;
bool g_rebuild_flag = false;
bool g_making_cpp = false;
bool g_print_colors = true;
const char* g_target = 0;
const char* HOME = 0;

void parse_options(int argc, char* argv[]);

// some extra lua functions
static int s_getcwd(lua_State* L)
{
	char cwd[260];
	getcwd(cwd, sizeof(cwd));
	lua_pushstring(L, cwd);
	return 1;
}

static int s_chdir(lua_State* L)
{
	if (lua_gettop(L) != 1 || !lua_isstring(L, -1))
		return luaL_error(L, "expected string");
	const char* str = lua_tostring(L, -1);
	if (chdir(str) != 0)
		return luaL_error(L, "chdir() failed");
	return 0;
}

int main(int argc, char* argv[])
{
	
	parse_options(argc, argv);
	HOME = file::getEnv("HOME");

	lua_State* L = lua_open();
	luaL_openlibs(L);

	lua_register(L, "getcwd", s_getcwd);
	lua_register(L, "chdir", s_chdir);
	
    project_init(L);

	if (!file::exists(g_makefile))
	{
		int nloops = 8;
		do
		{
			char cwd[260];
			getcwd(cwd, sizeof(cwd));

			char parent[260];
			strcpy(parent, cwd);
			char* slash = strrchr(parent, '/');
			if (slash && *(slash+1))
				*slash = 0;

			if (!strchr(parent, '/'))
				break;
			
			chdir(parent);

			nloops--;
		} while (nloops && !file::exists(g_makefile));
	}
	
	file::set_makefile_time(g_makefile);

    if (!g_target)
      g_target = "debug";
    
    lua_pushstring(L, g_target);
    lua_setglobal(L, "target");

	strfmt<512> autocake("%s/.cake/auto.lua", HOME);
	if (HOME && file::exists(autocake))
	{
		if (luaL_dofile (L, autocake))
		{
			printerr("%s", lua_tostring(L, -1));
		}
	}
	else if (g_verbose_flag)
	{
		printf("%s not found.\n", (const char*)autocake);
	}
	
	if (luaL_dofile (L, g_makefile))
	{
		printerr("%s", lua_tostring(L, -1));
	}
	
    // now compile all projects
    for (size_t i = 0; i < g_projects.size(); ++i)
	{
		Project* p = g_projects[i];
		if (p->pre_build_step(L))
		{
			if (p->compile())
			{
				// what to do if post build step fails?
				p->post_build_step(L);
			}
		}
	}

    for (size_t i = 0; i < g_projects.size(); ++i)
      delete g_projects[i];

	lua_close(L);
    
	return 0;
}

static struct argp_option options[] = {
	{"verbose",  'v', 0,      0,  "Produce verbose output" },
	{"quiet",    'q', 0,      0,  "Don't produce any output" },
	{"silent",   's', 0,      OPTION_ALIAS },
	{"rebuild",  'r', 0,      0,  "Force complete rebuild"},
	{"clean",    'c', 0,      0,  "Remove all temporaries"},
	{"file",     'f', "FILE", 0,  "Makefile to use. Default is make.lua"},
	{"new",      'n', "FILE", 0,  "Generates a new makefile for this folder"},
	{"unprettify", 'u', 0,     0,  "Less colorful output"},
	{ 0 }
};

struct arguments
{
	char* target;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	arguments* args = (arguments*)state->input;
	
	switch (key)
	{
		case 'q':
		case 's':
			g_silent_flag = true;
			break;
		
		case 'v':
			g_verbose_flag = true;
			break;
			
		case 'r':
			g_rebuild_flag = true;
			break;
		
		case 'c': // clean
			//TODO
			printerr("no clean for you");
			break;
			
		case 'n':
			printerr("no new for you");
			break;
			
	case 'f':
			g_makefile = arg;
			break;

	case 'u':
		g_print_colors = false;
		break;
		
		
		case ARGP_KEY_ARG:
			if (state->arg_num > 0)
				argp_usage(state);
			args->target = arg;
			break;
		
		case ARGP_KEY_END:
			break;
			
		default:
			return ARGP_ERR_UNKNOWN;
	};
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, cake_doc };

void parse_options(int argc, char* argv[])
{
	arguments args;
	args.target = 0;
	argp_parse (&argp, argc, argv, 0, 0, &args);
	
	if (args.target)
		g_target = args.target;
}


