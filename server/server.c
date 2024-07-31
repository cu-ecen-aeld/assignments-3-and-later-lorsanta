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
#include <pthread.h>
#include "queue.h"

static int keep_accepting_connections = 1;
static pthread_mutex_t mutex;
static int aesdsocketdatafd;

struct slist_data_s {
	int fd;
	pthread_t thread;
	int completed;
	SLIST_ENTRY(slist_data_s) entries;
};

void termination_handler (int signum)
{
	keep_accepting_connections = 0;
}

void alarm_handler (int signum)
{
	char outstr[200];
	alarm(10);

	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);

	pthread_mutex_lock(&mutex);
	write(aesdsocketdatafd, "timestamp:", strlen("timestamp:"));
	write(aesdsocketdatafd, outstr, strlen(outstr));
	write(aesdsocketdatafd, "\n", 1);
	pthread_mutex_unlock(&mutex);
}

int read_packet(int fd)
{
	char buf[1024];
	char *packet = NULL;
	int j = 0;
	int found_newline = 0;

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
				pthread_mutex_lock(&mutex);
				lseek(aesdsocketdatafd, 0, SEEK_SET);
				while((r = read(aesdsocketdatafd, buf, 1024)) > 0)
				{
					write(fd, buf, r);
				}
				write(aesdsocketdatafd, packet, j);
				pthread_mutex_unlock(&mutex);
				write(fd, packet, j);
				found_newline = 1;
				break;
			}
		}
	}

	free(packet);

	return 1;
}

void* handle_clientfn(void *data)
{
	int keep_reading_packets = 1;
	struct slist_data_s *elem = (struct slist_data_s *)data;
	while(keep_reading_packets)
	{
		keep_reading_packets = read_packet(elem->fd) && keep_accepting_connections;
	}

	elem->completed = 1;

	return NULL;
}

int serverfn(void)
{
	int socketfd;

	openlog("aesdsocket", 0, LOG_USER);

	struct sigaction termination_action, alarm_action, old_action;

	termination_action.sa_handler = termination_handler;
	sigemptyset (&termination_action.sa_mask);
	termination_action.sa_flags = 0;

	alarm_action.sa_handler = alarm_handler;
	sigemptyset (&alarm_action.sa_mask);
	alarm_action.sa_flags = 0;

	sigaction (SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGINT, &termination_action, NULL);

	sigaction (SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGTERM, &termination_action, NULL);

	sigaction (SIGALRM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGALRM, &alarm_action, NULL);


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

	pthread_mutex_init(&mutex, NULL);

	SLIST_HEAD(slisthead, slist_data_s) thread_list = SLIST_HEAD_INITIALIZER(thread_list);
	SLIST_INIT(&thread_list);

	aesdsocketdatafd = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_APPEND, 0664);
	if(aesdsocketdatafd == -1) {
		perror("open /var/tmp/aesdsocketdata");
		return -1;
	}

	alarm(10);

	while(keep_accepting_connections)
	{
		char ipaddr[INET_ADDRSTRLEN];
		struct sockaddr_in client_addr;
		unsigned int sin_size = sizeof(client_addr);

		struct slist_data_s *list_elem = malloc(sizeof(struct slist_data_s));
		SLIST_INSERT_HEAD(&thread_list, list_elem, entries);

		list_elem->completed = 0;
		if((list_elem->fd = accept(socketfd, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
			if (errno == EINTR )
			{
				if(!keep_accepting_connections) break;
			}
			else
			{
				perror("accept");
				return -1;
			}
		}

		inet_ntop(AF_INET, &(client_addr.sin_addr), ipaddr, INET_ADDRSTRLEN);

		syslog(LOG_USER, "Accepted connection from %s", ipaddr);

		if(pthread_create(&list_elem->thread, NULL, &handle_clientfn, list_elem) != 0)
		{
			perror("pthread_create");
			return -1;
		}

		struct slist_data_s *plist_elem, *plist_elem_temp;
		SLIST_FOREACH_SAFE(plist_elem, &thread_list, entries, plist_elem_temp)
		{
			if(plist_elem->completed)
			{
				pthread_join(plist_elem->thread, NULL);
				close(plist_elem->fd);
				SLIST_REMOVE(&thread_list,  plist_elem, slist_data_s, entries);
				free(plist_elem);
				syslog(LOG_USER, "Closed connection from %s", ipaddr);
			}
		}
	}

	struct slist_data_s *plist_elem, *plist_elem_temp;
	SLIST_FOREACH_SAFE(plist_elem, &thread_list, entries, plist_elem_temp)
	{
		close(plist_elem->fd);
		SLIST_REMOVE(&thread_list,  plist_elem, slist_data_s, entries);
		free(plist_elem);
	}

	close(aesdsocketdatafd);
	pthread_mutex_destroy(&mutex);
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
