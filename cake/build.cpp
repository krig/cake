#include "stdafx.h"
#include "build.h"
#include "file.h"
#include "arglist.h"
#include "command.h"
#include "globber.h"
#include "project.h"

using namespace std;

static void mkobjname(char* tmp, const char* fname)
{
	strcpy(tmp, g_objdir);
	strcat(tmp, fname);
	int len = strlen(tmp);
	char* dot = strrchr(tmp, '.');

	if (dot && ((dot - tmp) < len - 1)) { dot[1] = 'o'; dot[2] = '\0'; }
}

static void mkdepname(char* tmp, const char* fname)
{
	strcpy(tmp, g_objdir);
	strcat(tmp, fname);
	int len = strlen(tmp);

    const char* hdrs[] = { "h", "hpp", "hh", "hxx", "inc" };
	if (endswith1of(tmp, hdrs, ASIZE(hdrs)))
	{
		tmp[len] = '.';
		tmp[len+1] = 'd';
		tmp[len+2] = '\0';
	}
	else
	{
		char* dot = strrchr(tmp, '.');

		if (dot && ((dot - tmp) < len - 1)) { dot[1] = 'd'; dot[2] = '\0'; }
	}
}

static void mkpchname(char* tmp, const char* fname)
{
	strcpy(tmp, g_objdir);
	strcat(tmp, fname);
	strcat(tmp, ".gch");
}

std::string::size_type count(std::string& haystack,
							 const std::string& needle)
{
	using std::string;
	typedef string::size_type sint;
	
	sint cnt = 0;

	if (!needle.empty())
	{
		sint pos1 = 0;
		sint pos2 = 0;
		const sint needle_len = needle.size();

		pos1 = haystack.find(needle, 0);
		while (pos1 != string::npos)
		{
			pos2 = pos1 + needle_len;
			pos1 = haystack.find(needle, pos2);
			++cnt;
		}
	}

	return cnt;
}

static int find_num_processors()
{
	arglist args;
	args << "cat";
	args << "/proc/cpuinfo";
	std::string cstdout;
	int code = 0;
	if (!ExecuteCommand(args.get(), HOME, &code, cstdout))
		return 1;

	return max(count(cstdout, "processor"), (size_t)1);
}

build::build()
{
	int nprocessors = find_num_processors();
	if (g_verbose_flag)
		printf("processors: %d\n", nprocessors);
	
	linklist.reserve(64);
	nprocessed = 0;
	nskipped = 0;
	ncompiled = 0;
	nfailed = 0;
	nlinked = 0;

	jobs.resize(nprocessors);
}

build::~build()
{
	if (g_verbose_flag)
	{
		verbed(nskipped, "skipped", YELLOW);
		verbed(ncompiled, "compiled", WHITE);
		verbed(nlinked, "built", GREEN);
	}

	verbed(nfailed, "failed", RED);
}

bool build::file_is_newer(const char* fname, const char* compared_to) const
{
	if (file::exists(compared_to))
		return file::is_changed(fname, compared_to);

	return true;
}

template <typename T>

struct array_holder
{
	array_holder(T* ary) : ptr(ary) {}

	~array_holder() { delete [] ptr; }

	T* ptr;
};

bool build::needs_rebuild(const char* fname, const char* objname)
{
	if (g_rebuild_flag)
		return true;

	// check for dependency information
	char dep[260];

	mkdepname(dep, fname);

	if (!file::exists(dep))
		return true;

	array_holder<char> contents(file::read_file(dep));

	if (contents.ptr == 0)
		return true;

	char* start = strchr(contents.ptr, ':');

	if (!start)
		return true;

	start++;

	const char* delim = " \\\n\r\t";

	char* token = strtok(start, delim);

	while (token)
	{
		if (file_is_newer(token, objname))
			return true;

		token = strtok(0, delim);
	}

	return false;
}

void build::start_build()
{
	start_jobs();
}

void build::compile(const char* fname, const char* cflags)
{
	globber g(fname);

	if (g.size())
	{
		for (int i = 0; i < g.size(); ++i)
			do_compile(g[i], cflags);
	}
	else
	{
		do_compile(fname, cflags);
	}
}

void build::do_compile(const char* fname, const char* cflags)
{
	// TODO: eh, everything?
	// gcc <compile_options> <cflags> -o <target> <file>

	nprocessed++;

	char objname[260];
	mkobjname(objname, fname);
	file::mkpath(objname);

	if (!needs_rebuild(fname, objname))
	{
		nskipped++;
		// still need to add this to link list... duh
		linklist.push_back(objname);
		return;
	}

	char cwd[260];

	getcwd(cwd, 260);

	arglist args;
	args << g_compiler;

	for (int i = 0; *(g_compile_options[i]); ++i)
		args << g_compile_options[i];

	if (m_pchname.empty() == false)
	{
		char ipch[260] = "-I";
		strcat(ipch, file::path(m_pchname.c_str()).c_str());
		args << ipch;
	}

	if (g_target && (strstr(g_target, "debug") != 0))
		args << "-D_DEBUG";
	else
		args << "-DNDEBUG";
	
	args.insert_spacedelim(cflags);

	args << fname << "-o" << objname;

	if (g_verbose_flag)
	{
		for (size_t i = 0; i < args.size(); ++i)
			printf("%s ", args[i]);

		printf("\n");

		//cprintf(BLUE, "%d: %s", nprocessed, fname);
	}
	else
		cprintf(BLUE, "%s", fname);

	add_compile_job(objname, args.get());
	/*
	int code = 0;

	if (!ExecuteCommand(args.get(), cwd, &code))
	{
		printerr("%s FAIL", fname);
		nfailed++;
	}
	else if (code != 0)
	{
		nfailed++;
	}
	else
	{
		linklist.push_back(objname);
		ncompiled++;
	}
	*/
}

bool build::link(Project* project, const char* target, const char* ldflags, const table::ptr& libs)
{
	complete_jobs();
	
	if (nfailed)
		return false;

	char outname[260];
	sprintf(outname, "%s%s", g_bindir, target);

	// no recompiles: check if target exists and is new, if so don't build it
	// if g_rebuild is set, nskipped will be 0
	if (((int)linklist.size() == nskipped) &&
		(file::exists(outname) &&
		 !file::is_older(outname)))
		return true;

	
	file::mkpath(outname);

	char cwd[260];

	getcwd(cwd, 260);

	// TODO: eh, everything?
	// gcc <ldflags> -o <target> <allobjfiles> <libraries>
	// g_link_options
	arglist args;

	args << g_compiler;

	for (int i = 0; *(g_link_options[i]); ++i)
		args << g_link_options[i];

	if (m_pchname.empty() == false)
	{
		char ipch[260] = "-I";
		strcat(ipch, file::path(m_pchname.c_str()).c_str());
		args << ipch;
	}

	args.insert_spacedelim(ldflags);

	args << "-o" << outname;

	for (size_t i = 0; i < linklist.size(); ++i)
		args << linklist[i].c_str();

	if (g_verbose_flag)
	{
		for (size_t i = 0; i < args.size(); ++i)
			printf("%s ", args[i]);

		printf("\n");

		//cprintf(BLUE, "%d: %s", nprocessed, fname);
	}

	if (libs)
	{
		for (size_t i = 0; i < libs->strings.size(); ++i)
			args << libs->strings[i].c_str();
	}

	if (project->m_depends.size())
	{
		args << strfmt<64>("-L%s", g_libdir);
		for (size_t i = 0; i < project->m_depends.size(); ++i)
		{
			if (project->m_depends[i]->m_type != Project::PROGRAM)
			{
				char tmp[260];
				sprintf(tmp, "-l%s%s",
						project->m_depends[i]->m_name.c_str(),
						project->m_depends[i]->m_output_suffix.c_str());
				args << tmp;
			}
		}
		
	}

	int code = 0;

	if (!ExecuteCommand(args.get(), cwd, &code))
	{
		printerr("%s FAIL", target);
		nfailed++;
		return false;
	}
	else if (code != 0)
	{
		// do nothing, gcc reports error...
		nfailed++;
		return false;
	}
	else
	{
		cprintf(GREEN, "built %s.", target);
		nlinked++;
		return true;
	}
}

void build::make_pch(const char* pch, const char* cflags, bool cplusplus)
{

	// pch name is <pch>.gch
	char outputname[260];
	mkpchname(outputname, pch);

	file::mkpath(outputname);

	// depname for pch is special: basically obj/<pch>.d -
	// so obj/stdafx.d, for example
	char depname[260];
	mkdepname(depname, pch);

	m_pchname = outputname;

	// check if it exists, check for dependencies

	if (file::exists(outputname))
		if (needs_rebuild(pch, outputname) == false)
			return;

	// no? build!
	char cwd[260];

	getcwd(cwd, 260);

	arglist args;

	args << g_compiler;

	for (int i = 0; *(g_pch_options[i]); ++i)
		args << g_pch_options[i];

	args.insert_spacedelim(cflags);

	if (g_target && (strstr(g_target, "debug") != 0))
		args << "-D_DEBUG";
	else
		args << "-DNDEBUG";
	
	
	if (cplusplus)
	  args << "-x" << "c++-header";

	// move dependency output to obj/ dir
	// "-MF <file>"

	//args << "-MF" << depname << pch;
	args << "-o" << outputname << pch;

	if (g_verbose_flag)
	{
		for (size_t i = 0; i < args.size(); ++i)
			printf("%s ", args[i]);

		printf("\n");

		//cprintf(BLUE, "%d: %s", nprocessed, pch);
	}
	else
		cprintf(BLUE, "%s", pch);

	int code = 0;

	if (!ExecuteCommand(args.get(), cwd, &code))
	{
		printerr("%s FAIL", pch);
		nfailed++;
	}
	else if (code != 0)
	{
		nfailed++;
	}
	else
	{
//  linklist.push_back(objname);
		ncompiled++;
	}
}

bool build::staticlib(const char* target)
{
	complete_jobs();
	
	if (nfailed)
		return false;
	
	// outputname will be <lib>/lib<target>.a
	char outname[260];
	sprintf(outname, "%slib%s.a", g_libdir, target);

	// if no recompiles,
	// if file exists and is new, don't build it
	if (((int)linklist.size() == nskipped) &&
		(file::exists(outname) &&
		 !file::is_older(outname)))
		return true;

	file::mkpath(outname);

	char cwd[260];
	getcwd(cwd, 260);

	arglist args;
	args << "ar";
	args << "rcs";
	args << outname;
	
	for (size_t i = 0; i < linklist.size(); ++i)
		args << linklist[i].c_str();

	if (g_verbose_flag)
	{
		for (size_t i = 0; i < args.size(); ++i)
			printf("%s ", args[i]);
		printf("\n");
	}

	int code = 0;
	if (!ExecuteCommand(args.get(), cwd, &code))
	{
		printerr("%s FAIL", target);
		nfailed++;
		return false;
	}
	else if (code != 0)
	{
		nfailed++;
		return false;
	}
	else
	{
		cprintf(GREEN, "built %s.", target);
		nlinked++;
		return true;
	}
	
}

typedef unsigned int uint32;

static uint32 frand()
{
	const uint32 m = 2147483647;
	const uint32 a = 16807;
	static uint32 seed = 1;
	seed = (seed*a) % m;
	return seed;
}

void build::start_jobs()
{
}

void build::add_compile_job(const char* objname, char* const* argv)
{
	// find a free job, if none are available,
	// wait for one randomly and steal its slot

	for (size_t i = 0; i < jobs.size(); ++i)
	{
		if (jobs[i].cmd.is_active())
		{
			if (jobs[i].cmd.query_complete())
			{
				if (jobs[i].cmd.exitcode)
				{
					nfailed++;
				}
				else
				{
					linklist.push_back(jobs[i].objname);
					ncompiled++;
				}
				
				jobs[i].objname = objname;
				jobs[i].cmd.begin(argv, "./");
				return;
			}
		}
		else
		{
			jobs[i].objname = objname;
			jobs[i].cmd.begin(argv, "./");
			return;
		}
	}

	// no free slot found, wait for a random job
	size_t r = frand()%jobs.size();
	if (!jobs[r].cmd.wait())
	{
		nfailed++;
	}
	else if (jobs[r].cmd.exitcode)
	{
		nfailed++;
	}
	else
	{
		linklist.push_back(jobs[r].objname);
		ncompiled++;
	}
	jobs[r].objname = objname;
	jobs[r].cmd.begin(argv, "./");
}

void build::complete_jobs()
{
	for (size_t i = 0; i < jobs.size(); ++i)
	{
		if (jobs[i].cmd.is_active())
		{
			if (!jobs[i].cmd.wait())
			{
				nfailed++;
			}
			else if (jobs[i].cmd.exitcode)
			{
				nfailed++;
			}
			else
			{
				linklist.push_back(jobs[i].objname);
				ncompiled++;
			}
		}
	}
}

