/*
** aesdsocket
* Based on https://beej.us/guide/bgnet/html/#a-simple-stream-server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>

#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 100 // max number of bytes we can get at once

#define DATAFILE "/var/tmp/aesdsocketdata"

void sig_handler(int s) {
	if(remove(DATAFILE) != 0) {
		perror("remove");
	}
	syslog(LOG_DEBUG, "Caught signal, exiting\n");
	closelog();
	exit(EXIT_SUCCESS);
}

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv)
{
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	if(argc == 2) {
		if(strncmp(argv[1], "-d", 2) == 0) {
			if(daemon(0,0) != 0) {
				exit(EXIT_FAILURE);
			}
		} else {
			printf("unsupported options\n");
			exit(EXIT_FAILURE);
		}
	} else if (argc > 2) {
		printf("unsupported options\n");
		exit(EXIT_FAILURE);
	}

	openlog(NULL, 0, LOG_USER);

	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	memset(s, 0, sizeof(s));
	int rv;
	int numbytes;
	char buf[MAXDATASIZE];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_DEBUG, "Accepted connection from %s\n", s);

	while(1) {  // main accept() loop

		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		syslog(LOG_DEBUG, "Accepted connection from %s\n", s);

		if (!fork()) { // this is the child process

			FILE* file = fopen(DATAFILE, "a");

			close(sockfd); // child doesn't need the listener

			while(1) {

				memset(buf, 0, sizeof(buf));

				if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
					perror("recv");
					exit(EXIT_FAILURE);
				} else if (numbytes == 0) {
					break;
				} else {
					fprintf(file, "%s", buf);
				}
				if(buf[numbytes-1] == '\n') {
					break;
				}
			}

			fclose(file);

			file = fopen(DATAFILE, "r");
			char* c;
			char buf[MAXDATASIZE];
			while((c = fgets(buf, MAXDATASIZE, file)) != NULL) {
				send(new_fd, buf, strnlen(buf, MAXDATASIZE), 0);
			}

			fclose(file);

			close(new_fd);

			syslog(LOG_DEBUG, "Closed connection from %s\n", s);

			exit(EXIT_SUCCESS);
		}

		close(new_fd);  // parent doesn't need this
	}

	exit(EXIT_SUCCESS);
}

