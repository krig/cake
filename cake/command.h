#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

// NOTE: argv[0] must be the program!

struct Command
{
	Command();

	bool is_active();
	void begin(char* const* argv, const char* workdir);
	void begin_grab_stdout(char* const* argv, const char* workdir);
	bool query_complete();
	bool wait();

	// reads from child stdout
	int read_stdout(char* to, int bufsize);

	int exitcode;
	
private:
	pid_t pid;
	int stdout_pipe[2]; // pipe to read from process
};

bool ExecuteCommand(char* const* argv
                    , const char* workdir
                    , int* exitcode);

bool ExecuteCommand(char* const* argv
                    , const char* workdir
                    , int* exitcode
                    , std::string& output);


#endif /* __COMMAND_HPP__ */

