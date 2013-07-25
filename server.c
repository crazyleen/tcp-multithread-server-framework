#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "misc.h"
#include "socket.h"

#define Trace(...);    do { printf("%s:%u:TRACE: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define Debug(...);    do { printf("%s:%u:DEBUG: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define Info(...);		do { printf("%s:%u:INFO: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define Error(...);   	do { printf("%s:%u:ERROR: ", __FILE__, __LINE__); printf(__VA_ARGS__); perror(""); printf(" [%s]\n", __FUNCTION__); } while (0);

#define SEVER_PORT 5008
#define MAX_CLIENT 32

struct client_args {
	int sockfd;
	struct sockaddr_in addr;
};

static sem_t client_sem; //max connections
static int sockfd;	//server socket

/**
 * malloc args for thread
 * return args, NULL for malloc error
 */
static void *thread_alloc_args(struct client_args *client) {
	void *args = malloc(sizeof(struct client_args));
	if(args == NULL) {
		Error("malloc");
		return NULL;
	}
	memcpy(args, client, sizeof(struct client_args));
	return args;
}

static void thread_free_args(void *args) {
	if(args != NULL)
		free(args);
}

static void thread_client_cleanup(void *args) {
	struct client_args *client = (struct client_args*)args;
	//client thread exit
	if(sem_post(&client_sem) < 0) {
		perror("sem_wait");
	}
	close(client->sockfd);
	thread_free_args(args);
}

static void *thread_client(void *args) {
	struct client_args *client = (struct client_args*)args;
	pthread_cleanup_push(thread_client_cleanup, args);

	//TODO: client thread main
	char msg[1024] = "";
	if(recv(client->sockfd, msg, sizeof(msg), 0) > 0)
		printf("CLIENT# %s\n", msg);

	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}

static void at_exit_cleanup(void) {
	sem_destroy(&client_sem);
	close(sockfd);
}

int main(int argc, char**argv) {
	const char *pidfile = "/var/run/mspdebug-gdb-proxy.pid";
	if(write_pid_lock(pidfile) < 0) {
		printf("%s: already running\n", argv[0]);
		return 0;
	}

	sockfd = socket_server(SEVER_PORT, MAX_CLIENT);
	if (sockfd < 0) {
		Error("socket server");
		return -1;
	}

	if(sem_init(&client_sem, 0, MAX_CLIENT) < 0) {
		close(sockfd);
		Error("exit now");
		return -1;
	}

	atexit(at_exit_cleanup);
	while (1) {
		int client_addr_len;
		struct client_args client;
		pthread_t thread;
		int ret;

		if(sem_wait(&client_sem) < 0) {
			perror("sem_wait");
			break;
		}

		bzero(&client, sizeof(client));
		client_addr_len = sizeof(client.addr);
		client.sockfd = accept(sockfd, (struct sockaddr *) &client.addr,
						&client_addr_len);
		if (client.sockfd < 0) {
			Error("accept");
			break;
		}
		//client count

		Info("client connect from %s", inet_ntoa(client.addr.sin_addr));
		void *thread_args = thread_alloc_args(&client);
		ret = pthread_create(&thread, NULL, thread_client, thread_args);
		if (ret != 0) {
			Error("pthread_create");
			if(thread_args != NULL)
				free(thread_args);
			if(sem_post(&client_sem) < 0) {
				Error("sem_wait");
			}
			continue;
		}
		pthread_detach(thread);
	}

	return 0;
}
