#include <stdlib.h>
#include <unistd.h>
#include "systemcalls.h"
#include "sys/wait.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
	const int ret = system(cmd);
    return ret == 0;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

	int status;
	pid_t pid;
	pid = fork();

	if (pid == -1) { // fork failed
		return false;
	} else if (pid == 0) { // child
		execv(command[0], command);
		exit (-1); // if execv failed
	}

	if (wait(&status) == -1) {
		return false; // if child failed to exec
	} else if (WIFEXITED (status)) {
		return WEXITSTATUS (status) == 0;
	}

    va_end(args);

	return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

	// following https://stackoverflow.com/questions/13784269/redirection-inside-call-to-execvp-not-working/13784315#13784315
	int kidpid;
	int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	if (fd < 0) { perror("open"); abort(); }
	switch (kidpid = fork()) {
	case -1: return false;
	case 0:
		if (dup2(fd, 1) < 0) { perror("dup2"); abort(); }
		close(fd);
		execv(command[0], command);
		exit(-1);
	default:
		close(fd);
		int status;
		if (wait(&status) == -1) {
			return false; // if child failed to exec
		} else if (WIFEXITED (status)) {
			return WEXITSTATUS (status) == 0;
		}
	}

    va_end(args);

    return false;
}
