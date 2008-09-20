#include "stdafx.h"
#include "file.h"

namespace file
{
	time_t makefile_time = 0;
	time_t newest_time = 0;

	void set_makefile_time(const char* filename)
	{
		struct stat fstat;
		if (::stat(filename, &fstat) != 0)
		{
			printerr("ERROR: Unable to access %s for date comparison.",
			        filename);
			return;
		}
		makefile_time = fstat.st_mtime;
	}

	bool exists(const char* filename)
	{

		struct stat s;
		return ::stat(filename, &s) == 0;
	}

	const char* getEnv(const char* var)
	{
		return getenv(var);
	}

	void setEnv(const char* var, const char* val)
	{
		setenv(var, val, true);
	}

	bool makedir(const char* dir)
	{
		return mkdir(dir, 0755) == 0;
	}

	bool is_changed(const char* filename, const char* compare)
	{

		struct stat fstat;

		if (::stat(filename, &fstat) != 0)
		{
			printerr("ERROR: Unable to access %s for date comparison.",
			        filename);

			return true;
		}

		struct stat cstat;

		if (::stat(compare, &cstat) != 0)
		{
			printerr("ERROR: Unable to access %s for date comparison.",
			        compare);

			return true;
		}

		// if the makefile has changed, force rebuild
		if (makefile_time > cstat.st_mtime)
			return true;

		if (fstat.st_mtime > newest_time)
			newest_time = fstat.st_mtime;

		return fstat.st_mtime > cstat.st_mtime;
	}
	
	bool is_older(const char* filename)
	{
		struct stat fstat;
		if (::stat(filename, &fstat) != 0)
		{
			printerr("ERROR: Unable to access %s for date comparison.",
			        filename);
			return true;
		}

		// if the makefile has changed, force rebuild
		if (makefile_time > fstat.st_mtime)
			return true;

		return newest_time > fstat.st_mtime;
	}

	const char* basename(const char* full_path)
	{
		const char* slash = strrchr(full_path, '/');

		if (slash && *(slash + 1))
			return slash + 1;

		return full_path;
	}

	const char* extension(const char* full_path)
	{
		const char* dot = strrchr(full_path, '.');

		if (dot && *(dot + 1))
			return dot + 1;

		return 0;
	}

	std::string path(const char* full_path)
	{
		if (is_dir(full_path))
			return full_path;

		const char* base = basename(full_path);

		if (base == full_path)
			return full_path;

		if (base == 0)
			return "";

		// strip slash at end?
		return std::string(full_path, base - full_path);
	}

	bool is_dir(const char* fname)
	{

		struct stat s;

		if (stat(fname, &s) == 0)
			return (S_ISDIR(s.st_mode) != 0);

		return false;
	}


	namespace
	{
		char* trim(char* str, char* chars)
		{
			if (!str)
				return 0;

			char* end = strchr(str, 0) - 1;

			while (end >= str && strchr(chars, *end))
			{
				*end = 0;
				end--;
			}

			return str;
		}
	}

	void mkpath(const char* filename)
	{
		if (filename == 0)
			return;

		std::string xpath(path(filename));

		if (exists(xpath.c_str()))
		{
			if (!is_dir(xpath.c_str()))
			{
				printerr("ERROR: trying to create dir '%s', but it's a file!", filename);
				return;
			}

			return;
		}

		// grab first part, create it if needed; move string pointer forward, loop
		std::string currpart;

		const char* at = xpath.c_str();

		while (*at)
		{
			const char* next = strchr(at, '/');

			if (next)
			{
				currpart.assign(xpath.c_str(), next - xpath.c_str());
				makedir(currpart.c_str());
			}
			else // last part!
			{
				makedir(at);
				break;
			}

			at = next + 1;
		}
	}
	
	char* read_file(const char* fname)
	{
		FILE* f = fopen(fname, "r");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			long size = ftell(f);
			fseek(f, 0, SEEK_SET);
			if (size > 0)
			{
				char* contents = new char[size+1];
				int read = fread(contents, 1, size, f);
				contents[read] = 0;
				return contents;
			}
			fclose(f);
		}
		return 0;
	}
}

