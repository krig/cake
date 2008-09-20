#include "stdafx.h"
#include "project.h"
#include "file.h"
#include "command.h"
#include "arglist.h"
#include "build.h"

std::vector<Project*> g_projects;

// first parameter is table,
// rest are strings
// table contains hidden userdata pointer to Project
// of course!

static Project* s_getproject(lua_State* L, int tableindex = 1)
{
    lua_pushstring(L, "__project");
    lua_gettable(L, tableindex);
    Project* p = (Project*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return p;
}

static int s_add_libs(lua_State* L)
{
    Project* p = s_getproject(L);

    for (int i = 2; i <= lua_gettop(L); ++i)
    {
        const char* curr = lua_tostring(L, i);
        p->m_libs.push_back(curr);
    }

    return 0;
}
static int s_add_pkgdepend(lua_State* L)
{
    Project* p = s_getproject(L);

    for (int i = 2; i <= lua_gettop(L); ++i)
    {
        const char* curr = lua_tostring(L, i);
        p->m_pkgdepends.push_back(curr);
    }
    return 0;
}
static int s_add_flags(lua_State* L)
{
    Project* p = s_getproject(L);

    for (int i = 2; i <= lua_gettop(L); ++i)
    {
        const char* curr = lua_tostring(L, i);
        p->m_flags.push_back(curr);
    }

    return 0;
}
static int s_add_files(lua_State* L)
{
    Project* p = s_getproject(L);

    for (int i = 2; i <= lua_gettop(L); ++i)
    {
        const char* curr = lua_tostring(L, i);
        p->m_files.push_back(curr);
    }
    return 0;
}
static int s_set_output_suffix(lua_State* L)
{
    Project* p = s_getproject(L);
    p->m_output_suffix = lua_tostring(L, 2);
    return 0;
}

static int s_depend(lua_State* L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, "expected a parameter to depend()");
    }

    Project* p = s_getproject(L);
    Project* dep = s_getproject(L, 2);

    p->m_depends.push_back(dep);

    return 0;
}

#define REG_LUA_FUNC(name)                                      \
    { lua_pushstring(L, #name); lua_pushcfunction(L, s_##name); \
        lua_settable(L, -3); }

static int s_create_program(lua_State* L)
{
    if (lua_gettop(L) != 1)
        luaL_error(L, "Wrong number of parameters "
                   "to program(). "
                   "Expected string (name)");
    if (!lua_isstring(L, 1))
        luaL_error(L, "Expected string (name)");
    const char* name = lua_tostring(L, 1);
    for (size_t i = 0; i < g_projects.size(); ++i)
        if (g_projects[i]->m_name == name)
        {
            luaL_error(L, "project '%s' already exists", name);
        }

    Project* p = new Project;
    p->m_name = name;
    p->m_type = Project::PROGRAM;

    // now the tricky part: make
    // a lua table containing the project
    // functions, and return that (and save
    // a link to it in the project class)
    lua_newtable(L);
    REG_LUA_FUNC(add_libs);
    REG_LUA_FUNC(add_pkgdepend);
    REG_LUA_FUNC(add_flags);
    REG_LUA_FUNC(add_files);
    REG_LUA_FUNC(set_output_suffix);
    REG_LUA_FUNC(depend);

    // add secret userdata pointer to table
    lua_pushstring(L, "__project");
    lua_pushlightuserdata(L, p);
    lua_settable(L, -3);

    // save an entry in the registry
    // with this as key and the table as value
    //lua_pushlightuserdata(L, p);
    //lua_pushvalue(L, 2); // copy table to top of stack
    //lua_settable(L, LUA_REGISTRYINDEX); // set entry in registry
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushvalue(L, 2);
    p->m_L = L;
    p->m_registrykey = luaL_ref(L, LUA_REGISTRYINDEX);

    g_projects.push_back(p);

    return 1;
}

static int s_create_library(lua_State* L)
{
    if (lua_gettop(L) != 1)
        luaL_error(L, "Wrong number of parameters "
                   "to library(). "
                   "Expected string (name)");
    if (!lua_isstring(L, 1))
        luaL_error(L, "Expected string (name)");
    const char* name = lua_tostring(L, 1);

    for (size_t i = 0; i < g_projects.size(); ++i)
        if (g_projects[i]->m_name == name)
        {
            luaL_error(L, "project '%s' already exists", name);
        }
    Project* p = new Project;
    p->m_name = name;
    p->m_type = Project::STATIC_LIBRARY;

    lua_newtable(L);
//  REG_LUA_FUNC(add_libs);
    REG_LUA_FUNC(add_pkgdepend);
    REG_LUA_FUNC(add_flags);
    REG_LUA_FUNC(add_files);
    REG_LUA_FUNC(set_output_suffix);

    lua_pushstring(L, "__project");
    lua_pushlightuserdata(L, p);
    lua_settable(L, -3);

    // save an entry in the registry
    // with this as key and the table as value
    //lua_pushlightuserdata(L, p);
    //lua_pushvalue(L, 2); // copy table to top of stack
    //lua_settable(L, LUA_REGISTRYINDEX); // set entry in registry
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushvalue(L, 2);
    p->m_L = L;
    p->m_registrykey = luaL_ref(L, LUA_REGISTRYINDEX);

    g_projects.push_back(p);

    return 1;
}

void project_init(lua_State* L)
{
    lua_pushcfunction(L, s_create_program);
    lua_setglobal(L, "program");

    lua_pushcfunction(L, s_create_library);
    lua_setglobal(L, "library");
}

bool is_header(const char* fname)
{
    static const char* hdr[] = { ".h", ".hpp", ".hxx", ".inc" };
    return endswith1of(fname, hdr, ASIZE(hdr));
}

bool is_cppfile(const char* fname)
{
    const char* cpe[] = {"cpp", "cxx", "cc", "C"};
    return endswith1of(fname, cpe, ASIZE(cpe));
}

extern bool g_making_cpp;

Project::Project()
{
    m_L = 0;
    m_registrykey = 0;
}

Project::~Project()
{
    if (m_L)
    {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_registrykey);
    }
}

bool Project::compile()
{
    build b;

    std::string flags;

    // process pkgdepends
    //if (m_type == PROGRAM)
    {
        using namespace std;
        char cwd[260];
        getcwd(cwd, 260);

        for (size_t i = 0; i < m_pkgdepends.size(); ++i)
        {
            const char* pkg = m_pkgdepends[i].c_str();

            arglist args;
            args << "pkg-config" << "--exists" << pkg;
            int code = -1;
            if (!ExecuteCommand(args.get(), "./", &code))
            {
                printerr("Failed to run pkgconfig");
                return false;
            }
            if (code != 0)
            {
                printerr("Missing dependency %s", pkg);
            }

            std::string cstdout;
            args.clear();
            args << "pkg-config" << "--cflags" << pkg;
            code = -1;
            if (!ExecuteCommand(args.get(), cwd, &code, cstdout))
            {
                printerr("failed running pkg-config");
                return false;
            }

            flags += cstdout;

            args.clear();
            args << "pkg-config" << "--libs" << pkg;
            code = -1;
            if (!ExecuteCommand(args.get(), cwd, &code, cstdout))
            {
                printerr("Failed");
                return false;
            }

            args.clear();
            args.insert_spacedelim(cstdout.c_str());
            for (size_t i = 0; i < args.size(); ++i)
            {
                m_libs.push_back(args[i]);
            }
        }
    }


    for (size_t i = 0; i < m_flags.size(); ++i)
        flags += " " + m_flags[i];

    g_making_cpp = false;
    for (size_t i = 0; i < m_files.size(); ++i)
        if (is_cppfile(m_files[i].c_str()))
        {
            g_making_cpp = true;
            break;
        }

    // make sure obj directory exists
    {
        std::string newobjdir(g_baseobjdir);
        newobjdir += g_target;
        newobjdir += "/";
        strcpy(g_objdir, newobjdir.c_str());
        file::mkpath(g_objdir);
    }

    b.start_build();

    // compile precompiled header
    for (size_t i = 0; i < m_files.size(); ++i)
    {
        if (is_header(m_files[i].c_str()))
        {
            b.make_pch(m_files[i].c_str(), flags.c_str(), g_making_cpp);
            break;
        }
    }

    // compile the rest
    for (size_t i = 0; i < m_files.size(); ++i)
    {
        if (!is_header(m_files[i].c_str()))
        {
            b.compile(m_files[i].c_str(), flags.c_str());
        }
    }

    bool retval = false;

    if (m_type == PROGRAM)
    {
        table::ptr libs(new table);
        for (size_t i = 0; i < m_libs.size(); ++i)
            libs->strings.push_back(m_libs[i]);
        if (g_making_cpp)
            libs->strings.push_back("-lstdc++");

        std::string tgt(m_name + m_output_suffix);
        if (tgt.empty() == false)
        {
            retval = b.link(this, tgt.c_str(), flags.c_str(), libs);
        }
    }
    else if (m_type == STATIC_LIBRARY)
    {
        std::string tgt(m_name + m_output_suffix);
        if (tgt.empty() == false)
        {
            retval = b.staticlib(tgt.c_str());
        }
    }
    else
    {
        printerr("not supported.\n");
    }

    return retval;
}

bool Project::pre_build_step(lua_State* L)
{
    return do_build_step(L, "pre");
}

bool Project::post_build_step(lua_State* L)
{
    return do_build_step(L, "post");
}

bool Project::do_build_step(lua_State* L, const char* step)
{
#define lua_swap(L) lua_insert(L, -2)

    if (!m_L || L != m_L)
    {
        printerr("this is just plain wrong.");
        return false;
    }



    //lua_pushlightuserdata(L, this);
    //lua_gettable(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, m_registrykey);
    int t = lua_gettop(L);

    if (t < 1)
    {
        printerr("%s: no project pointer found in registry for %p / %d", step, this, m_registrykey);
        return false;
    }

    lua_pushnil(L);  /* first key */
    while (lua_next(L, t) != 0)
    {
        if (lua_isstring(L, -2))
        {
            if (strcmp(step, lua_tostring(L, -2)) == 0)
            {
                break;
            }
        }
        lua_pop(L, 1);
    }
    // if we found post, it's on the top of the stack
    bool bigret = true;
    if (lua_gettop(L) > t)
    {
        lua_swap(L);
        lua_pop(L, 1);
        lua_swap(L);
        // woot! now the table is on the top, and the function just below it.
        // lets call!
        int ret = lua_pcall(L, 1, 1, 0);
        if (ret)
        {
            printerr(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        else
        {
			
            if (lua_isboolean(L, -1))
                bigret = lua_toboolean(L, -1);
            lua_pop(L, 1);
        }
		if (lua_gettop(L) > t)
			lua_pop(L, 1);
    }
    return bigret;
}
