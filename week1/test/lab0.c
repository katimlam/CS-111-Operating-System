#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>

static int segfault_flag = 0;
static int catch_flag = 0;
static char *input_file = NULL;
static char *output_file = NULL;

static struct option long_options[]=
{
	{ "segfault", no_argument, &segfault_flag, 1 },
	{ "catch", no_argument, &catch_flag, 1},
	{ "input", required_argument, NULL, 'i'},
	{ "output", required_argument, NULL, 'o' }
};

void sighandler(int signum);

int main(int argc, char *argv[])
{
	/*deal with command line arguments*/
		int c;
		int ifd, ofd;
		extern char *optarg;
		extern int optind, optopt;
		while ((c = getopt_long(argc, argv, "i:o:sc", long_options, NULL)) != -1)
		{
			switch (c)
			{
			case 's':
				segfault_flag = 1;
				break;
			case 'c':
				catch_flag = 1;
				break;
			case 'i':
				//if (optarg != "--output" && optarg != "--segfault" && optarg != "--catch")
				input_file = optarg;
				break;
			case 'o':
				//if (optarg != "--input" && optarg != "--segfault" && optarg != "--catch")
				output_file = optarg;
				break;
			case ':':
				exit(3);
				break;
			case '?':
				exit(4);
				break;
			}
		}

		/*enable segfault handler*/
		if (catch_flag)
		{
			signal(SIGSEGV, sighandler);
		}

		/*segfault and catch*/
		if (segfault_flag)
		{
			char * p = NULL;
			*p = 's';
		}

		/*open file descriptor*/
		
		if (input_file != NULL)
		{
			int ifd = open(input_file, O_RDONLY);
			if (ifd < 0)
			{
				perror("Cannot open input file");
				exit(1);
			}
			dup2(ifd, 0);
			close(ifd);
		}

		if (output_file != NULL)
		{
			int ofd = creat(output_file, 0666);
			if (ofd < 0)
			{
				perror("Cannot open output file");
				exit(2);
			}
			dup2(ofd, 1);
			close(ofd);
		}

		/*read and write fd*/
		char buf[1];
		while ( read(0, buf, 1) > 0)
		{
			write(1, buf, 1);
		}
		close(0);
		close(1);
		exit(0);
}

void sighandler(int signum)
{
	fprintf(stderr, "Segmentation fault with error %d.\n", signum);
	exit(3);
}