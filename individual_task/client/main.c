#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_SIZE 50

int main(int argc, char** args)
{
	const char* hostIP;
	const char* portStr;
	if (argc == 1) {
		hostIP = "127.0.0.1";
		portStr = "3000";
	} else if (argc == 3) {
		hostIP = args[1];
		portStr = args[2];
	} else {
		printf("Use \"%s <host_ip> <host_port>\"\n", args[0]);
		return 1;
	}
	
    int sock_desc;
    struct sockaddr_in serv_addr;
    char cmd[MAX_SIZE],answer[MAX_SIZE];

    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(hostIP);
    serv_addr.sin_port = htons(atoi(portStr));

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
		return 1;
    }

    printf("Connected successfully client:\n");
	int n;
    while(1)
    {
        printf("Message to send: ");
        fgets(cmd, MAX_SIZE, stdin);
        send(sock_desc,cmd,strlen(cmd),0);
		if (!strcmp(cmd, "quit\n")) {
			printf("Bye!\n");
			break;
		}
		if (!strcmp(cmd, "put\n")) {
			uint64_t len;
			printf("Title: ");
			fgets(cmd, MAX_SIZE, stdin);
			len = strlen(cmd) - 1;
			send(sock_desc, &len, sizeof(uint64_t), 0);
			send(sock_desc, cmd, len, 0);
			printf("Author: ");
			fgets(cmd, MAX_SIZE, stdin);
			len = strlen(cmd) - 1;
			send(sock_desc, &len, sizeof(uint64_t), 0);
			send(sock_desc, cmd, len, 0);
			printf("Article content: ");
			do {
				fgets(cmd, MAX_SIZE, stdin);
				len = strlen(cmd);
				send(sock_desc, &len, sizeof(uint64_t), 0);
				send(sock_desc, cmd, len, 0);
			} while (strcmp(cmd, "\n"));
		}
        while ((n=recv(sock_desc,answer,MAX_SIZE - 1,0)) > 0) {
			answer[n] = '\0';
			//printf("answer = %s\n", answer);
			fputs(answer,stdout);
			if (n >= 2 && answer[n - 2] == '\0' && answer[n - 1] == '\n') {
				break;
			}
		}
		if (n < 0) {
			printf("The server was turned off\n");
			break;
		}
		if (n == 0) {
			printf("n == 0\n");
		}

        bzero(answer,MAX_SIZE);
    }
    close(sock_desc);
    return 0;
}

