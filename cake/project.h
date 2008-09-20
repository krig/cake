#ifndef __PROJECT_H__
#define __PROJECT_H__

struct Project
{
	enum Type
	{
		PROGRAM,
		STATIC_LIBRARY,
		SHARED_LIBRARY
	};

	Project();
	~Project();
	
	Type m_type;

	lua_State* m_L;
	int m_registrykey;
	
	std::vector<std::string> m_files;
	std::vector<std::string> m_libs;
	std::vector<std::string> m_flags;
	std::vector<std::string> m_pkgdepends;
	std::vector<Project*> m_depends;
	std::string m_name;
	std::string m_output_suffix;

	bool compile();
	bool pre_build_step(lua_State* L);
	bool post_build_step(lua_State* L);

	bool do_build_step(lua_State* L, const char* step);
};

// registers the project functions with lua
void project_init(lua_State* L);

extern std::vector<Project*> g_projects;

#endif /* __PROJECT_H__ */

