#ifndef __BUILD_HPP__
#define __BUILD_HPP__

struct Project;

#include "command.h"

// TODO: compile multiple objects at once
// jobs are tricky: need to store exit code
// internally (and anything that depends on
// reading output can't use them)

struct build
{
	build();
	~build();

	bool needs_rebuild(const char* fname, const char* objname);

	void start_build();
	
	void make_pch(const char* pch, const char* cflags, bool cplusplus);
	void compile(const char* fname, const char* cflags);

	bool link(Project* p, const char* target, const char* ldflags, const table::ptr& libs);
	bool staticlib(const char* target);

private:
	void do_compile(const char* fname, const char* cflags);
	bool file_is_newer(const char* fname, const char* compared_to) const;

	void start_jobs();
	void add_compile_job(const char* objname,
		char* const* argv);
	void complete_jobs();

	struct Job
	{
		Command cmd;
		std::string objname;
	};
	
	std::vector<Job> jobs;
	
	int nprocessed;
	int nskipped;
	int ncompiled;
	int nfailed;
	int nlinked;
	std::vector<std::string> linklist;
	std::string m_pchname;
};

#endif /* __BUILD_HPP__ */

