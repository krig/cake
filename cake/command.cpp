#include "stdafx.h"
#include "command.h"

Command::Command()
{
	pid = 0;
	exitcode = 0;
}

void Command::begin(char* const* argv, const char* workdir)
{
	exitcode = 0;
	stdout_pipe[0] = -1;
	stdout_pipe[1] = -1;

	pid = fork();

	if (pid == 0)
	{
		if (chdir(workdir) == -1)
		{
			printerr("ERROR: Unable to change to directory %s.",
			        workdir);
		}

		execvp(argv[0], argv);

		// If we got here, it means the command didn't execute
		printerr("ERROR: Unable to run %s.", argv[0]);
		exit(-1);
	}
}

void Command::begin_grab_stdout(char* const* argv, const char* workdir)
{
	exitcode = 0;
	if (pipe(stdout_pipe) < 0)
	{
		printerr("can't make pipe");
		exit(-1);
	}
	
	pid = fork();

	// Child.
	if (pid == 0)
	{
		if (chdir(workdir) == -1)
		{
			printerr("ERROR: Unable to change to directory %s.",
			        workdir);
		}
		
		close(1); // Close current stdout.
		dup( stdout_pipe[1]); // Make stdout go to write end of pipe.		

		close( stdout_pipe[0]);

		execvp(argv[0], argv);

		// If we got here, it means the command didn't execute
		printerr("ERROR: Unable to run %s.", argv[0]);
		exit(-1);
	}
	// parent
	else if (pid != -1)
	{
		close(stdout_pipe[1]);
	}
}

int Command::read_stdout(char* to, int bufsize)
{
	return read(stdout_pipe[0], to, bufsize);
}

bool Command::is_active()
{
	return pid != 0;
}

bool Command::query_complete()
{
	if (!is_active())
		return true;
	
	int status = 0;
	bool done = waitpid(pid, &status, WNOHANG) == pid;
	if (done)
	{
		exitcode = WEXITSTATUS(status);
		pid = 0;
	}
	return done;
}

bool Command::wait()
{
	if (!is_active())
		return true;
	
	int status = 0;

	if (waitpid(pid, &status, 0) == pid)
	{
		exitcode = WEXITSTATUS(status);
		pid = 0;
		return true;
	}
	else
	{
		pid = 0;
		return false;
	}
}

bool ExecuteCommand(char* const* argv, const char* workdir, int* exitcode)
{
	Command cmd;
	cmd.begin(argv, workdir);
	bool ok = cmd.wait();
	if (exitcode)
		*exitcode = cmd.exitcode;
	return ok;
}

bool ExecuteCommand(char* const* argv
                    , const char* workdir
                    , int* exitcode
                    , std::string& output)
{
	output = "";
	Command cmd;
	cmd.begin_grab_stdout(argv, workdir);
	
	char tmp[513];
	int ret = 0;
	do
	{
		ret = cmd.read_stdout(tmp, sizeof(tmp)-1);
		if (ret > 0)
		{
			tmp[ret] = 0;
			output += tmp;
		}
	} while (ret > 0);
	
	bool ok = cmd.wait();
	if (exitcode)
		*exitcode = cmd.exitcode;
	return ok;
}

