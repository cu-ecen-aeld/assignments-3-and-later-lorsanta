#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

static int keep_accepting_connections = 1;

void termination_handler (int signum)
{
	keep_accepting_connections = 0;
}

int read_packet(int fd)
{
	char buf[1024];
	char *packet = NULL;
	int j = 0;
	int found_newline = 0;

	int aesdsocketdatafd = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_APPEND, 0664);
	if(aesdsocketdatafd == -1) {
		perror("open /var/tmp/aesdsocketdata");
		return -1;
	}

	lseek(aesdsocketdatafd, 0, SEEK_SET);

	while(!found_newline)
	{
		int read_bytes = read(fd, buf, 1024);
		if(read_bytes <= 0) return 0;

		packet = realloc(packet, j + read_bytes);
		memcpy(packet + j, buf, read_bytes);
		j += read_bytes;
		
		for(int i = 0; i < read_bytes; i++)
		{
			if(buf[i] == '\n') {
				int r;
				while((r = read(aesdsocketdatafd, buf, 1024)) > 0)
				{
					write(fd, buf, r);
				}
				write(fd, packet, j);
				write(aesdsocketdatafd, packet, j);
				found_newline = 1;
				break;
			}
		}
	}

	free(packet);
	close(aesdsocketdatafd);

	return 1;
}

int serverfn(void)
{
	int socketfd;

	openlog("aesdsocket", 0, LOG_USER);


	struct sigaction new_action, old_action;

	/* Set up the structure to specify the new action. */
	new_action.sa_handler = termination_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction (SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
	sigaction (SIGINT, &new_action, NULL);

	sigaction (SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
	sigaction (SIGTERM, &new_action, NULL);


	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}
	int val = 1;
	setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(9000);

	if(bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("bind");
		return -1;
	}

	if(listen(socketfd, SOMAXCONN) == -1)
	{
		perror("listen");
		return -1;
	}

	while(keep_accepting_connections)
	{
		int fd;
		char ipaddr[INET_ADDRSTRLEN];
		struct sockaddr_in client_addr;
		unsigned int sin_size = sizeof(client_addr);
		if((fd = accept(socketfd, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
			if (errno == EINTR) break;
			perror("accept");
			return -1;
		}
		inet_ntop(AF_INET, &(client_addr.sin_addr), ipaddr, INET_ADDRSTRLEN);
		
		syslog(LOG_USER, "Accepted connection from %s", ipaddr);
		int keep_reading_packets = 1;
		while(keep_reading_packets)
		{
			keep_reading_packets = read_packet(fd);
		}

		syslog(LOG_USER, "Closed connection from %s", ipaddr);

		close(fd);
	}
	
	close(socketfd);
	unlink("/var/tmp/aesdsocketdata");
	closelog();

	return 0;
}

int main(int argc, char *argv[])
{
	int start_as_deamon = 0;
		
	if(argc >= 2) {
		start_as_deamon = strcmp(argv[1], "-d") == 0 ? 1 : 0;
	}
	
	if(start_as_deamon)
	{
		pid_t pid1 = fork();
		if(pid1 == 0)
		{
			setsid();
			return serverfn();
		}
	}
	else
	{
		return serverfn();
	}
	
	return 0;
}
