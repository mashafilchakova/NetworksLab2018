#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "line.h"
#include "cmd_common.h"
#include "block.h"

#define ERR (-1)
#define PTR_ERR NULL
#define CWD_MAX_LEN 1000

char curDir[CWD_MAX_LEN] = "/";

// get command from terminal or -1 on error
static cmd_t getCmd(char **arg);

// create datagram from command and argument; and set the length of
// it in gramLen
// return NULL on error; otherwise on the heap allocated datagram
static void *makeRequest(cmd_t cmd, const char *arg, int *requestLen);

static void *makeLsRequest(int *requestLen);

static void *makeCdRequest(const char *arg, int *requestLen);

static void *makeGetRequest(const char *arg, int *requestLen);

static void *makeGetByAuthorRequest(const char *arg, int *requestLen);

static void *makePutRequest(int *requestLen);

static char *getContent();

// return: NULL if it is the last datagram for sending
// otherwise, return pointer to datagram for sending
void *getRequest(int *gramLen) {
    cmd_t cmd;
    char *cmdArg;
    void *res;

    while (1) {
        cmd = getCmd(&cmdArg);
        if (cmd == ERR || cmd == CMD_QUIT) {
            return PTR_ERR;
        }
        if (cmd == CMD_UNKNOWN) {
            puts("Unknown command");
            puts("Please, use one of the following: ls, cd <sec>, get <art>, put, quit");
            continue;
        }
        res = makeRequest(cmd, cmdArg, gramLen);
        if (cmdArg) {
            free(cmdArg);
        }
        return res;
    }
}

static cmd_t getCmd(char **arg) {
    char *line;
    cmd_t res = CMD_UNKNOWN;
    char *argStart = NULL;

    *arg = NULL;
    printf("Input command (ls, cd <sec>, get <art>, put, quit, getByAuthor <author>): ");
    line = getLine();
    if (line == NULL) {
        return ERR;
    }
    if (!strcmp(line, "ls")) {
        res = CMD_LS;
    } else if (strstr(line, "cd ") == line) {
        res = CMD_CD;
        argStart = line + strlen("cd ");
    } else if (strstr(line, "get ") == line) {
        res = CMD_GET;
        argStart = line + strlen("get ");
    } else if (!strcmp(line, "put")) {
        res = CMD_PUT;
    } else if (!strcmp(line, "quit")) {
        res = CMD_QUIT;
    } else if (strstr(line, "getByAuthor ") == line) {
        res = CMD_GET_BY_AUTHOR;
        argStart = line + strlen("getByAuthor ");
    }
    if (argStart) {
        *arg = (char *) malloc(strlen(argStart) + 1);
        if (!*arg) {
            res = ERR;
        } else {
            strcpy(*arg, argStart);
        }
    }
    free(line);
    return res;
}

static void *makeRequest(cmd_t cmd, const char *arg, int *requestLen) {
    if (cmd == CMD_LS) {
        return makeLsRequest(requestLen);
    }
    if (cmd == CMD_CD) {
        return makeCdRequest(arg, requestLen);
    }
    if (cmd == CMD_GET) {
        return makeGetRequest(arg, requestLen);
    }
    if (cmd == CMD_PUT) {
        return makePutRequest(requestLen);
    }
    if (cmd == CMD_GET_BY_AUTHOR) {
        return makeGetByAuthorRequest(arg, requestLen);
    }
    return PTR_ERR;
}

static void *makeLsRequest(int *requestLen) {
    gram_wrapper_t *gramWrapper;
    cmd_t lsCmd = CMD_LS;
    void *res;
    gramWrapper = gramWrapperInit(&lsCmd, CMD_LEN);
    if (!gramWrapper) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, curDir) == ERR) {
        return PTR_ERR;
    }
    res = gramWrapper->gram;
    *requestLen = gramWrapper->gramLen;
    free(gramWrapper);
    return res;
}

static void *makeGetByAuthorRequest(const char *arg, int *requestLen) {
    gram_wrapper_t *gramWrapper;
    cmd_t getByAuthorCmd = CMD_GET_BY_AUTHOR;
    void *res;

    gramWrapper = gramWrapperInit(&getByAuthorCmd, CMD_LEN);
    if (!gramWrapper) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, curDir) == ERR) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, arg) == ERR) {
        return PTR_ERR;
    }
    res = gramWrapper->gram;
    *requestLen = gramWrapper->gramLen;
    free(gramWrapper);
    return res;
}

static void *makeCdRequest(const char *arg, int *requestLen) {
    gram_wrapper_t *gramWrapper;
    cmd_t cdCmd = CMD_CD;
    void *res;

    gramWrapper = gramWrapperInit(&cdCmd, CMD_LEN);
    if (!gramWrapper) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, curDir) == ERR) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, arg) == ERR) {
        return PTR_ERR;
    }
    res = gramWrapper->gram;
    *requestLen = gramWrapper->gramLen;
    free(gramWrapper);
    return res;
}

static void *makeGetRequest(const char *arg, int *requestLen) {
    gram_wrapper_t *gramWrapper;
    cmd_t getCmd = CMD_GET;
    void *res;

    gramWrapper = gramWrapperInit(&getCmd, CMD_LEN);
    if (!gramWrapper) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, curDir) == ERR) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, arg) == ERR) {
        return PTR_ERR;
    }
    res = gramWrapper->gram;
    *requestLen = gramWrapper->gramLen;
    free(gramWrapper);
    return res;
}

static void *makePutRequest(int *requestLen) {
    gram_wrapper_t *gramWrapper;
    cmd_t putCmd = CMD_PUT;
    void *res;
    char *title, *author, *content;
    int addBlockRes = 0;

    gramWrapper = gramWrapperInit(&putCmd, CMD_LEN);
    if (!gramWrapper) {
        return PTR_ERR;
    }
    if (charBlockAdd(gramWrapper, curDir) == ERR) {
        return PTR_ERR;
    }
    printf("Title: ");
    title = getLine();
    printf("Author: ");
    author = getLine();
    printf("Article content (use empty line for the end of content): ");
    content = getContent();
    if (title && author && content) {
        addBlockRes = charBlockAdd(gramWrapper, title);
        if (addBlockRes != ERR) {
            addBlockRes = charBlockAdd(gramWrapper, author);
        }
        if (addBlockRes != ERR) {
            addBlockRes = charBlockAdd(gramWrapper, content);
        }
    }
    free(title);
    free(author);
    free(content);
    if (addBlockRes == ERR) {
        return PTR_ERR;
    }
    res = gramWrapper->gram;
    *requestLen = gramWrapper->gramLen;
    free(gramWrapper);
    return res;
}

static char *getContent() {
    char *line = PTR_ERR;
    char *res = PTR_ERR;
    int size;
    void *reallocAnswer;
    int success = 0;

    while ((line = getLineNL()) != PTR_ERR) {
        if (strlen(line) == 1 && line[0] == '\n') {
            success = 1;
            break;
        }
        size = 0;
        if (res) {
            size = strlen(res);
        }
        reallocAnswer = realloc(res, size + strlen(line) + 1);
        if (!reallocAnswer) {
            success = 0;
            break;
        }
        res = reallocAnswer;
        memcpy(res + size, line, strlen(line) + 1);
    }
    if (success) {
        free(line);
        return res;
    }
    if (line) {
        free(line);
    }
    if (res) {
        free(res);
    }
    return PTR_ERR;
}

