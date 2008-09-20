#ifndef __FILE_H__
#define __FILE_H__

namespace file
{
	void set_makefile_time(const char* filename);

	bool exists(const char* filename);
	const char* getEnv(const char* var);

	void setEnv(const char* var, const char* val);

	bool is_changed(const char* filename, const char* compare);

	bool is_older(const char* filename);

	// returns only the filename part
	const char* basename(const char* full_path);
	
	// returns the extension
	const char* extension(const char* full_path);
	
	// strips the filename, leaving only the path
	std::string path(const char* full_path);

	bool is_dir(const char* fname);

	bool makedir(const char* dir);

	// makes sure all of the directories in the path exists
	void mkpath(const char* filename);
	
	char* read_file(const char* fname);

}

#endif /* __FILE_H__ */

