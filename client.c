#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "socket.h"

int main(int argc, char**argv) {
	int sockfd;

	if(argc != 3) {
		fprintf(stderr, "USAGE: %s <ip address> <message>\n", argv[0]);
		return -1;
	}

	if ((sockfd = socket_connect_dst(argv[1], 5008)) < 0) {
		perror("socket_connect_dst error\n");
		goto main_exit;
	}

	int len = strlen(argv[2]) + 1;
	if(send(sockfd, argv[2], len, 0) != len)
		perror("send");

main_exit:
	close(sockfd);
	return 0;
}
