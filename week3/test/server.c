#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <string.h>
#include <mcrypt.h>
#include <fcntl.h>

void reset_attributes(void);
void set_attr(void);
void *thread_write(void *arg);
void sig_func_SIGPIPE(int sig_num);
void print_exit_status(void);
void error3(char* errmsg);
int str2int(char* str);

pid_t pid;
int sockfd;
MCRYPT td;
static struct termios saved_attributes;
static int encrypt_flag = 0;
static struct option long_options[] =
{
	{ "port", required_argument, NULL, 'p' },
	{"encrypt", no_argument, &encrypt_flag, 1}
};

int main(int argc, char *argv[])
{
	/*parsing argument "shell".*/
	int c;
	char* argument = NULL;
	extern char *optarg;
	extern int optind, optopt;
	while ((c = getopt_long(argc, argv, "s", long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'p':
			argument = optarg;
			break;
		case ':':
			exit(3);
			break;
		case '?':
			exit(3);
			break;
		}
	}
	
	/*dealing with tormios*/
	//set_attr();
	/*exit handling*/
	//atexit(reset_attributes);

	/*Connection*/
	int newsockfd, portno, clilen;
	if (argument == NULL)
		error3("No port provided");
	portno = str2int(argument);
	struct sockaddr_in serv_addr, cli_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error3("Error opening socket");
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error3("ERROR on binding");
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0)
		error3("ERROR on accept");
	/////////////////////////////////////////////////////
	/*Mcrypt*/
	if (encrypt_flag)
	{
		char key[16];
		int key_fd = open("my.key", O_RDONLY);
		if (key_fd < 0)
			error3("Cannot open my.key");
		int keysize = read(key_fd, key, sizeof(key));
		td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		if (td == MCRYPT_FAILED)
			error3("ERROR on mcrypt_module_open");

		int IVsize = mcrypt_enc_get_iv_size(td);
		char* IV = malloc(IVsize);
		/*use port number as seed*/
		srand((unsigned int)portno);
		/*generate secret number for each char of IV between 0 to 255*/
		int i;
		for (i = 0; i < IVsize; i++)
		{
			IV[i] = rand();
			//printf("Server IV[%d]: %d\n", i, IV[i]);
		}
		i = mcrypt_generic_init(td, key, keysize, IV);
		if (i < 0)
		{
			mcrypt_perror(i);
			exit(3);
		}
		free(IV);
	}
	/////////////////////////////////////////////////////
	/*create two pipes*/
	int terminal_to_shell[2];
	int shell_to_terminal[2];
	pipe(terminal_to_shell);
	pipe(shell_to_terminal);

	/*fork new process*/
	pid = fork();
	if (pid < (pid_t) 0)
		error3("ERROR on Fork");
	if (pid == (pid_t) 0)                  /*shell process*/
	{
		//perror("in child");
		/*pipe redirection*/
		close(terminal_to_shell[1]);
		dup2(terminal_to_shell[0], STDIN_FILENO);
		close(terminal_to_shell[0]);
		close(shell_to_terminal[0]);
		dup2(shell_to_terminal[1], STDOUT_FILENO);
		dup2(shell_to_terminal[1], STDERR_FILENO);
		close(shell_to_terminal[1]);
		/*execl*/
		execl("/bin/bash", "/bin/bash", NULL);
	}
	else                                 /*parent process*/
	{
		/*piping*/
		close(terminal_to_shell[0]);
		close(shell_to_terminal[1]);
		int to_shell = terminal_to_shell[1];
		int from_shell = shell_to_terminal[0];
		/*socket redirection*/
		dup2(newsockfd, STDIN_FILENO);
		dup2(newsockfd, STDOUT_FILENO);
		dup2(newsockfd, STDERR_FILENO);
		close(newsockfd);
		/*thread for write*/
		pthread_t thread_2;
		if (pthread_create(&thread_2, NULL, thread_write, &from_shell) != 0)
			error3("Error in pthread_create");
		/*thread for read*/
		/*read input from the connection write to a child.*/
		char buf[1];
		int n;
		while ( (n = read(STDIN_FILENO, buf, sizeof(buf)) ) > 0)
		{
			if (encrypt_flag)
			{
				mdecrypt_generic(td, buf, n);
			}
			if (buf[0] == '\r' || buf[0] == '\n')
			{
				buf[0] = '\n';
			}
			else if (buf[0] == 0x3)
			{
				kill(pid, SIGINT);
			}
			else if (buf[0] == 0x4)
			{
				kill(pid, SIGHUP);
				close(to_shell);
				close(from_shell);
				exit(0);
			}
			write(to_shell, buf, sizeof(buf));
		}
		if (n <= 0)
		{
			close(sockfd);
			kill(pid, SIGKILL);
			exit(1);
		}
		exit(0);
	}
	return 0;
}

void *thread_write(void *arg)   //read from child ouput to connection
{
	signal(SIGPIPE, sig_func_SIGPIPE);
	int fd = *(int *)arg;
	int n;
	char buf[256];
	while ( (n =  read(fd, buf, sizeof(buf))) > 0)
	{
		if (encrypt_flag)
		{
			mcrypt_generic(td, buf, n);
		}
		write(STDOUT_FILENO, buf, n);
	}
	if (n < 0)
	{
		close(sockfd);
		exit(2);
	}
	return NULL;
}
void set_attr(void)
{
	/*save original terminal settinngs.*/
	if (tcgetattr(STDIN_FILENO, &saved_attributes) < 0)
		error3("Error in tcgetattr");

	/*put the console into character-at-a-time, no-echo mode
	(also known as non-canonical input mode with no echo). */
	struct termios attributes;
	if (tcgetattr(STDIN_FILENO, &attributes) < 0)
		error3("Error in tcgetattr");

	attributes.c_lflag &= ~(ICANON | ECHO);
	attributes.c_cc[VMIN] = 1;
	attributes.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &attributes) < 0) //if optional_actions is TCSANOW, the change shall occur immediately.
		error3("Error in tcsetattr");
}
void reset_attributes(void)
{
	//perror("server has reset attribute");
	if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes) < 0)
		error3("Error in tcsetattr");
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
	close(sockfd);
	exit(2);
}
void error3(char* errmsg)
{
	perror(errmsg);
	exit(3);
}
int str2int(char* str)
{
	for (int i = 0; str[i]!='\0'; i++)
	{
		switch (str[i])
		{
		case'0':
		case'1':
		case'2':
		case'3':
		case'4':
		case'5':
		case'6':
		case'7':
		case'8':
		case'9':
			break;
		default:
			error3("Invalid port");
			return -1;
			break;
		}
	}
	
	long val;
	char *endptr = NULL;
	val = strtol(str, &endptr, 10);
	return (int) val;	
	
}