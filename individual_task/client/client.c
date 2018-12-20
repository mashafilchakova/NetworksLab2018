#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "gram_common.h"
#include "request.h"
#include "response_applier.h"

#define ERR (-1)
#define PTR_ERR NULL

typedef struct sockaddr SA;

int sock;
struct sockaddr_in serverAddr;
socklen_t serverAddrLen;

// returns 0 if it is the last message; otherwise, another value
int processMsg();

int main(int argc, char **argv) {
    const char *serverIP = "127.0.0.1";
    
    serverAddrLen = sizeof(serverAddr);
    bzero(&serverAddr, serverAddrLen);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3000);
    if (!inet_aton(serverIP, &serverAddr.sin_addr)) {
        printf("Failed to use server IP-address = %s\n", serverIP);
        return EXIT_FAILURE;
    }
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Failed to create an endpoint");
        return EXIT_FAILURE;
    }
    while (processMsg() != ERR) {
    }
    puts("Bye");
    return EXIT_SUCCESS;
}

int processMsg() {
    void *request;
    int n, len;
    char response[MAX_GRAM];

    if ((request = getRequest(&len)) == PTR_ERR) {
        return ERR;
    }
    n = sendto(sock, request, len, 0, (struct sockaddr*)&serverAddr, serverAddrLen);
    free(request);
    if (n == ERR) {
        puts("Failed to send the message");
        return ERR;
    }
    if ((n = recvfrom(sock, response, MAX_GRAM, 0, (SA*)&serverAddr, &serverAddrLen)) == ERR) {
        perror("Failed to receive the message");
        return ERR;
    }
    if (n == 5 && response[4] == '\0' && !strcmp(response, "quit")) {
        puts("You were disconnected from the server, Bye");
        exit(0);
        return ERR;
    }
    if (applyResponse(response, n) == ERR) {
        puts("Failed to display the server's answer; sorry");
        return ERR;
    }
    return 0;
}

