#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

void reset_attributes(void);
void set_attr(void);
void *thread_write(void *arg);
void sig_func_SIGPIPE(int sig_num);
void print_exit_status(void);

pid_t pid;
static struct termios saved_attributes;
static int shell_flag = 0;
static struct option long_options[] =
{
	{"shell", no_argument, &shell_flag, 1}
};

int main(int argc, char *argv[])
{
	/*parsing argument "shell".*/
	int c;
	extern char *optarg;
	extern int optind, optopt;
	while ( (c=getopt_long(argc, argv, "s", long_options, NULL)) != -1){
		if (c == '?')
		{
			perror("Invalid Option");
			exit(3);
		}
	}
	/*dealing with tormios*/
	set_attr();

	if (shell_flag)   /*shell flag is on*/
	{
		/*create two pipes*/
		int terminal_to_shell[2];
		int shell_to_terminal[2];
		pipe(terminal_to_shell);
		pipe(shell_to_terminal);
		/*fork new process*/
		pid = fork();
		if (pid < (pid_t) 0)
		{
			perror("Error in Fork()");
			exit(3);
		}
		if (pid == (pid_t) 0)                  /*shell process*/
		{
			//perror("in child");
			/*pipe redirection*/
			close(terminal_to_shell[1]);
			dup2(terminal_to_shell[0], STDIN_FILENO);
			close(shell_to_terminal[0]);
			dup2(shell_to_terminal[1], STDOUT_FILENO);
			/*execl*/
			execl("/bin/bash", "/bin/bash", NULL);
		}
		else                                 /*parent process*/
		{
			signal(SIGPIPE, sig_func_SIGPIPE);
			/*exit handling*/
			atexit(print_exit_status);
			atexit(reset_attributes);
			/*piping*/
			close(terminal_to_shell[0]);
			close(shell_to_terminal[1]);
			int to_shell = terminal_to_shell[1];
			int from_shell = shell_to_terminal[0];
			/*thread for write*/
			pthread_t thread_2;
			if (pthread_create(&thread_2, NULL, thread_write, &from_shell) != 0)
			{
				perror("Error in pthread_create");
				exit(3);
			}
			/*thread for read*/
			/*read input from the keyboard into a buffer.*/
			char buf[1];
			while (read(STDIN_FILENO, buf, sizeof(buf)) > 0)
			{
				if (buf[0] == '\r' || buf[0] == '\n')
				{
					buf[0] = '\r';
					write(STDOUT_FILENO, buf, sizeof(buf));
					buf[0] = '\n';
				}
				else if (buf[0] == 0x3)
				{
					kill(pid, SIGINT);
				}
				else if (buf[0] == 0x4)
				{
					close(to_shell);
					close(from_shell);
					kill(pid, SIGHUP);
					exit(0);
				}
				write(STDOUT_FILENO, buf, sizeof(buf));
				write(to_shell, buf, sizeof(buf));
			}
			exit(0);
		}
	}
	else    /*shemll flag is off*/
	{
		atexit(reset_attributes);
		char buf[1];
		while (read(STDIN_FILENO, buf, sizeof(buf)) > 0)
		{
			if (buf[0] == '\r' || buf[0] == '\n')
			{
				char crlf_buf[2] = "\r\n";
				write(STDOUT_FILENO, crlf_buf, sizeof(crlf_buf));
			}
			else if (buf[0] == 0x4)
			{
				exit(0);
			}
			else
			{
				write(STDOUT_FILENO, buf, sizeof(buf));
			}
		}
		exit(0);
	}
	return(0);
}

void *thread_write(void *arg)
{
	int fd = *(int *)arg;
	char buf[1];
	while (read(fd, buf, sizeof(buf)) > 0)
	{
		if (buf[0] == 0x4)
		{
			exit(1);
		}
		write(STDOUT_FILENO, buf, sizeof(buf));
	}
	return NULL;
}
void set_attr(void)
{
	/*save original terminal settinngs.*/
	if (tcgetattr(STDIN_FILENO, &saved_attributes) < 0)
	{
		perror("Error in tcgetattr");
		exit(3);
	}

	/*put the console into character-at-a-time, no-echo mode
	(also known as non-canonical input mode with no echo). */
	struct termios attributes;
	if (tcgetattr(STDIN_FILENO, &attributes) < 0)
	{
		perror("Error in tcgetattr");
		exit(3);
	}
	attributes.c_lflag &= ~(ICANON | ECHO);
	attributes.c_cc[VMIN] = 1;
	attributes.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &attributes) < 0) //if optional_actions is TCSANOW, the change shall occur immediately.
	{
		perror("Error in tcsetattr");
		exit(3);
	}
}
void reset_attributes(void)
{
	//perror("reset");
	if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes) < 0)
	{
		perror("Error in tcsetattr");
		exit(3);
	}
}
void print_exit_status(void)
{
	/*check exit status of shell*/
	int status;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		printf("Exit status of shell: %d\n", WEXITSTATUS(status));
	else
		printf("Child process aborted normally.\n");
}
void sig_func_SIGPIPE(int sig_num)
{
	exit(1);
}