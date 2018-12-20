#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "gram_common.h"
#include "response.h"

#define ERR (-1)
#define PTR_ERR NULL

typedef struct sockaddr SA;

typedef struct {
    SA* clientAddr;
    socklen_t clientAddrLen;
    void *request;
    int requestLen;
} ThreadArg;

void *inputHandler(void *notUsed);
void *handler(void *varg);

ThreadArg *createThreadArg(void *request, int requestLen, SA *clientAddr, socklen_t clientAddrLen);

void releaseThreadArg(ThreadArg *arg);

int sock;

extern char root[];
int ports[1000];
int portCnt = 0;
pthread_mutex_t conLock;

int main(int argc, char **argv) {
    if (argc == 2) {
        if (!dirExists(argv[1])) {
            printf("Sorry, but the directory %s does not exist\n", argv[1]);
            return EXIT_FAILURE;
        }
        strcpy(root, argv[1]);
        if (root[strlen(root) - 1] == '/') {
            root[strlen(root) - 1] = '\0';
        }
    }
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;
    pthread_t tid;
    int n;
    
    if (pthread_mutex_init(&conLock, NULL)) {
        perror("Failed to initialize the mutex\n");
        return EXIT_FAILURE;
    }
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Failed to create an endpoint");
        return EXIT_FAILURE;
    }
    puts("Endpoint created");
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(3000);
    if (bind(sock, (SA*)&serverAddr, sizeof(serverAddr))) {
        perror("Failed to assign address and port to socket");
        return EXIT_FAILURE;
    }
    puts("Address and port assigned to socket");

    pthread_t inputTid;
    if (pthread_create(&inputTid, NULL, inputHandler, NULL)) {
        perror("Failed to create thread\n");
        return EXIT_FAILURE;
    }

    char request[MAX_GRAM];
    clientAddrLen = sizeof(clientAddr);
    while ((n = recvfrom(sock, request, MAX_GRAM, MSG_WAITALL, (SA*)&clientAddr, &clientAddrLen)) != -1) {
        ThreadArg *pArg = createThreadArg(request, n, (SA*)&clientAddr, clientAddrLen);
        if (pthread_create(&tid, NULL, handler, pArg)) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
        clientAddrLen = sizeof(clientAddr);
    }
    perror("hi");
    printf("%d\n", n);
    return EXIT_SUCCESS;
}

void *handler(void *varg) {
    pthread_detach(pthread_self());
    ThreadArg *arg = (ThreadArg *)varg;
    void *response;
    int responseLen;
    char *msgBad = "Bad request\n";
    int cliPort = ntohs(((struct sockaddr_in *) arg->clientAddr)->sin_port);
    int found = 0;
    int i;

    printf("%d\n", cliPort);
    pthread_mutex_lock(&conLock);
    for (i = 0; i < portCnt; i++) {
        if (ports[i] == cliPort) {
            found = 1;
            if (i != portCnt - 1) {
                int tmp = ports[i];
                ports[i] = ports[portCnt - 1];
                ports[portCnt - 1] = tmp;
            }
            portCnt--;
            break;
        }
    }
    pthread_mutex_unlock(&conLock);
    if (found) {
        response = (char *) malloc(strlen("quit") + 1);
        if (response) {
            strcpy(response, "quit");
            responseLen = 5;
        }
    } else {
        response = makeResponse(arg->request, arg->requestLen, &responseLen);
    }
    if (response == PTR_ERR) {
        response = (char *) malloc(strlen(msgBad) + 1);
        if (response) {
            strcpy(response, msgBad);
            responseLen = strlen(msgBad) + 1;
        }
    }
    if (response != PTR_ERR) {
        sendto(sock, response, responseLen, 0, arg->clientAddr, arg->clientAddrLen);
        free(response);
    }
    releaseThreadArg(arg);
    return 0;
}

ThreadArg *createThreadArg(void *request, int requestLen, SA *clientAddr, socklen_t clientAddrLen) {
    ThreadArg *res = (ThreadArg *)malloc(sizeof(ThreadArg));
    if (!res) {
        exit(EXIT_FAILURE);
    }
    res->requestLen = requestLen;
    res->clientAddrLen = clientAddrLen;
    if (!(res->request = (char *)malloc(requestLen))) {
        exit(EXIT_FAILURE);
    }
    if (!(res->clientAddr = (SA*)malloc(clientAddrLen))) {
        exit(EXIT_FAILURE);
    }
    memcpy(res->request, request, requestLen);
    memcpy(res->clientAddr, clientAddr, clientAddrLen);
    return res;
}

void releaseThreadArg(ThreadArg *arg) {
    free(arg->request);
    free(arg->clientAddr);
    free(arg);
}

void *inputHandler(void *notUsed) {
    (void) notUsed;
    int portNumber;

    pthread_detach(pthread_self());
    while (1) {
        scanf(" %d", &portNumber);
        pthread_mutex_lock(&conLock);
        ports[portCnt] = portNumber;
        portCnt++;
        pthread_mutex_unlock(&conLock);
    }
}
