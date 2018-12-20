#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include "response.h"
#include "cmd_common.h"
#include "string.h"
#include "stdlib.h"
#include "article.h"

#define ERR (-1)
#define PTR_ERR NULL
#define LEN_LEN (sizeof(uint32_t))
#define CWD_MAX 1000

char root[CWD_MAX] = "data";

static void *makeLsResponse(const char *requestGram, int requestLen, int *responseLen);
static void *makePutResponse(const char *requestGram, int requestLen, int *responseLen);
static void *makeGetByAuthorResponse(const char *requestGram, int requestLen, int *responseLen);
static void *makeCdResponse(const char *requestGram, int requestLen, int *responseLen);
static void *makeGetResponse(const char *requestGram, int requestLen, int *responseLen);

static void *doGetByAuthor(const char *cwd, const char *author);
static void *doLs(const char *cwd);
static void *doPut(const char *cwd, const char *title, const char *author, const char *content);
static void *doCd(const char *cwd, const char *section);
static void *doGet(const char *cwd, const char *article);

static int authorIsEqual(const char *pth, const char *title, const char *author);
static char *lsByAuthor(const char* dirPath, const char *author);
static char *ls(const char* dirPath);
int dirExists(const char* dirPath);
static int fileExists(const char* path);
// -1 on error
static int verifyCwd(const char *cwd);
static char *makePath(const char *cwd);

// make datagram with server-response from datagram with client-request
// return: NULL on error or pointer to autoallocated response if success
void *makeResponse(const void *requestGram, int requestLen, int *responseLen) {
    const char *request = (const char *) requestGram;
    cmd_t cmd;

    if (requestLen < CMD_LEN) {
        return PTR_ERR;
    }
    memcpy(&cmd, request, CMD_LEN);
    request += CMD_LEN;
    requestLen -= CMD_LEN;
    if (cmd == CMD_LS) {
        return makeLsResponse(request, requestLen, responseLen);
    } else if (cmd == CMD_PUT) {
        return makePutResponse(request, requestLen, responseLen);
    } else if (cmd == CMD_CD) {
        return makeCdResponse(request, requestLen, responseLen);
    } else if (cmd == CMD_GET) {
        return makeGetResponse(request, requestLen, responseLen);
    } else if (cmd == CMD_GET_BY_AUTHOR) {
        return makeGetByAuthorResponse(request, requestLen, responseLen);
    }
    puts("makeResponse() failed, no command");
    return PTR_ERR;
}

static void *makeLsResponse(const char *request, int requestLen, int *responseLen) {
    const char *cwd;
    int cwdLen;
    void *res;

    if (requestLen < LEN_LEN) {
        return PTR_ERR;
    }
    cwd = request;
    cwdLen = *((const int32_t *)cwd);
    cwd += LEN_LEN;
    if (LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    printf("%s\n", cwd);
    if (verifyCwd(cwd) == ERR) {
        return PTR_ERR;
    }
    res = doLs(cwd);
    if (!res) {
        return PTR_ERR;
    }
    *responseLen = strlen(res) + 1;
    return res;
}

static void *makePutResponse(const char *request, int requestLen, int *responseLen) {
    const char *cwd, *title, *author, *content;
    int cwdLen, titleLen, authorLen, contentLen;
    void *res;

    if (requestLen < LEN_LEN) {
        return PTR_ERR;
    }
    cwd = request;
    cwdLen = *((const int32_t *)cwd);
    cwd += LEN_LEN;
    if (LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    if (verifyCwd(cwd) == ERR) {
        return PTR_ERR;
    }

    if (2*LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    title = cwd + cwdLen;
    titleLen = *((const int32_t *)title);
    title += LEN_LEN;
    if (2*LEN_LEN + cwdLen + titleLen > requestLen) {
        return PTR_ERR;
    }

    if (3*LEN_LEN + cwdLen + titleLen > requestLen) {
        return PTR_ERR;
    }
    author = title + titleLen;
    authorLen = *((const int32_t *)author);
    author += LEN_LEN;
    if (3*LEN_LEN + cwdLen + titleLen + authorLen > requestLen) {
        return PTR_ERR;
    }

    if (4*LEN_LEN + cwdLen + titleLen + authorLen > requestLen) {
        return PTR_ERR;
    }
    content = author + authorLen;
    contentLen = *((const int32_t *)content);
    content += LEN_LEN;
    if (4*LEN_LEN + cwdLen + titleLen + authorLen + contentLen > requestLen) {
        return PTR_ERR;
    }
    res = doPut(cwd, title, author, content);
    if (!res) {
        return PTR_ERR;
    }
    *responseLen = strlen(res) + 1;
    return res;
}

static void *makeCdResponse(const char *request, int requestLen, int *responseLen) {
    const char *cwd, *section;
    int cwdLen, sectionLen;
    void *res;

    if (requestLen < LEN_LEN) {
        return PTR_ERR;
    }
    cwd = request;
    cwdLen = *((const int32_t *)cwd);
    cwd += LEN_LEN;
    if (LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    if (verifyCwd(cwd) == ERR) {
        return PTR_ERR;
    }

    if (2*LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    section = cwd + cwdLen;
    sectionLen = *((const int32_t *)section);
    section += LEN_LEN;
    if (2*LEN_LEN + cwdLen + sectionLen > requestLen) {
        return PTR_ERR;
    }
    res = doCd(cwd, section);
    if (!res) {
        return PTR_ERR;
    }
    *responseLen = strlen(res) + 1;
    return res;
}

static void *makeGetByAuthorResponse(const char *request, int requestLen, int *responseLen) {
    const char *cwd, *author;
    int cwdLen, authorLen;
    void *res;

    if (requestLen < LEN_LEN) {
        return PTR_ERR;
    }
    cwd = request;
    cwdLen = *((const int32_t *)cwd);
    cwd += LEN_LEN;
    if (LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    if (verifyCwd(cwd) == ERR) {
        return PTR_ERR;
    }

    if (2*LEN_LEN + cwdLen > requestLen) {
        return PTR_ERR;
    }
    author = cwd + cwdLen;
    authorLen = *((const int32_t *) author);
    author += LEN_LEN;
    if (2*LEN_LEN + cwdLen + authorLen > requestLen) {
        return PTR_ERR;
    }
    res = doGetByAuthor(cwd, author);
    if (!res) {
        return PTR_ERR;
    }
    *responseLen = strlen(res) + 1;
    return res;
}

static void *makeGetResponse(const char *request, int requestLen, int *responseLen) {
    const char *cwd, *article;
    int cwdLen, articleLen;
    void *res;

    if (requestLen < LEN_LEN) {
        return PTR_ERR;
    }
    cwd = request;
    cwdLen = *((const int32_t *)cwd);
    cwd += LEN_LEN;
    if (LEN_LEN + cwdLen > requestLen) {
        puts("makeGetResponse() failed, verify #1");
        return PTR_ERR;
    }
    if (verifyCwd(cwd) == ERR) {
        puts("makeGetResponse() failed, verify #2");
        return PTR_ERR;
    }

    if (2*LEN_LEN + cwdLen > requestLen) {
        puts("makeGetResponse() failed, verify #3");
        return PTR_ERR;
    }
    article = cwd + cwdLen;
    articleLen = *((const int32_t *)article);
    article += LEN_LEN;
    if (2*LEN_LEN + cwdLen + articleLen > requestLen) {
        puts("makeGetResponse() failed, verify #4");
        return PTR_ERR;
    }
    res = doGet(cwd, article);
    if (!res) {
        puts("makeGetResponse() failed, verify #5");
        return PTR_ERR;
    }
    *responseLen = strlen(res) + 1;
    return res;
}

static void *doGetByAuthor(const char *cwd, const char *author) {
    char *pth = makePath(cwd);
    void *res;
    if (!pth) {
        return PTR_ERR;
    }
    res = lsByAuthor(pth, author);
    free(pth);
    return res;
}

static void *doLs(const char *cwd) {
    char *pth = makePath(cwd);
    void *res;
    if (!pth) {
        return PTR_ERR;
    }
    res = ls(pth);
    free(pth);
    return res;
}

static void *doPut(const char *cwd, const char *title, const char *author, const char *content) {
    char *msgOk = "Article successfully saved\n";
    char *msgBad = "Failed to save the article\n";
    char *cwdPth = makePath(cwd);
    char *pth;
    char *res;
    FILE *f;

    if (!cwdPth) {
        puts("Bad cwdPth");
        return PTR_ERR;
    }
    pth = (char *) malloc(strlen(title) + 2 + strlen(cwdPth));
    if (!pth) {
        free(cwdPth);
        puts("malloc() in doPut()");
        return PTR_ERR;
    }
    strcpy(pth, cwdPth);
    strcat(pth, "/");
    strcat(pth, title);
    free(cwdPth);
    if (fileExists(pth)) {
        res = (char *) malloc(strlen(msgBad) + 1);
        if (!res) {
            free(pth);
            puts("malloc() #2 in doPut()");
            return PTR_ERR;
        }
        strcpy(res, msgBad);
        free(pth);
        return res;
    } else {
        int success = 1;
        if ((f = fopen(pth, "wb")) > 0) {
            if (fwrite(author, 1, strlen(author), f) <= 0) {
                success = 0;
            }
            if (fwrite("\n", 1, 1, f) <= 0) {
                success = 0;
            }
            if (fwrite(content, 1, strlen(content), f) <= 0) {
                success = 0;
            }
            fclose(f);
        } else {
            success = 0;
        }
        if (success) {
            res = (char *) malloc(strlen(msgOk) + 1);
            if (!res) {
                free(pth);
                return PTR_ERR;
            }
            strcpy(res, msgOk);
        } else {
            res = (char *) malloc(strlen(msgBad) + 1);
            if (!res) {
                free(pth);
                return PTR_ERR;
            }
            strcpy(res, msgBad);
        }
        free(pth);
        return res;
    }
    return PTR_ERR;
}

static void *doCd(const char *cwd, const char *section) {
    if (!*section || strstr(section, "/")) {
        return PTR_ERR;
    }
    char *cwdPth = (char *) malloc(strlen(cwd) + strlen(section) + 2);
    if (!cwdPth) {
        return PTR_ERR;
    }
    strcpy(cwdPth, cwd);
    strcat(cwdPth, section);
    strcat(cwdPth, "/");
    if (!strcmp(section, ".")) {
        strcpy(cwdPth, cwd);
    } else if (!strcmp(section, "..")) {
        strcpy(cwdPth, cwd);
        int i;
        for (i = strlen(cwdPth) - 2; i >= 0; i--) {
            if (cwdPth[i] == '/') {
                cwdPth[i + 1] = '\0';
                break;
            }
        }
    }

    char *pth = makePath(cwdPth);
    char *msgOk = "Current section is changed to: ";
    char *msgBad = "Failed to change current section\n";
    char *res;

    if (!pth) {
        free(cwdPth);
        return PTR_ERR;
    }
    if (dirExists(pth)) {
        res = (char *) malloc(strlen(cwdPth) + strlen(msgOk) + 2);
        if (!res) {
            free(pth);
            free(cwdPth);
            return PTR_ERR;
        }
        strcpy(res, msgOk);
        strcat(res, cwdPth);
        strcat(res, "\n");
    } else {
        res = (char *) malloc(strlen(msgBad) + 1);
        if (!res) {
            free(pth);
            free(cwdPth);
            return PTR_ERR;
        }
        strcpy(res, msgBad);
    }
    free(pth);
    free(cwdPth);
    return res;
}

static int authorIsEqual(const char *pth, const char *title, const char *author) {
    char* articleBuffer;
    int authorLen;
    char *author_;
    char* articleContent;
    char megaPth[1000];
    char authorHere[500];
    int res = 0;
    strcpy(megaPth, pth);
    strcat(megaPth, title);

    if (fileExists(megaPth) && readArticle(megaPth, &articleBuffer, &author_, &authorLen, &articleContent)) {
        memcpy(authorHere, author_, authorLen);
        authorHere[authorLen] = '\0';
        if (!strcmp(authorHere, author)) {
            res = 1;
        }
        releaseArticleBuffer(&articleBuffer);
    }
    return res;
}

static void *doGet(const char *cwd, const char *article) {
    char *res;
    char* articleBuffer;
    char* author;
    int authorLen;
    char* articleContent;
    char *msgBad = "System: Sorry, the article is corrupted\n";
    char *msgTitle = "Title: ";
    char *msgAuthor = "\nAuthor: ";
    char *msgContent = "\nArticle content:\n";
    char *cwdPth = makePath(cwd);
    char *pth;

    puts("get started");
    if (!cwdPth) {
        puts("Failed to make path");
        return PTR_ERR;
    }
    pth = (char *) malloc(strlen(article) + 1 + strlen(cwdPth));
    if (!pth) {
        free(cwdPth);
        puts("Failed to make path + article");
        return PTR_ERR;
    }
    strcpy(pth, cwdPth);
    strcat(pth, article);
    free(cwdPth);

    if (fileExists(pth) && readArticle(pth, &articleBuffer, &author, &authorLen, &articleContent)) {
        free(pth);
        res = (char *) malloc(strlen(msgTitle) + strlen(article) + strlen(msgAuthor) +
            authorLen + strlen(msgContent) + strlen(articleContent) + 1);
        if (res) {
            strcpy(res, msgTitle);
            strcat(res, article);
            strcat(res, msgAuthor);
            memcpy(res + strlen(msgTitle) + strlen(article) + strlen(msgAuthor), author, authorLen);
            *(res + strlen(msgTitle) + strlen(article) + strlen(msgAuthor) + authorLen) = '\0';
            strcat(res, msgContent);
            strcat(res, articleContent);
        }
        releaseArticleBuffer(&articleBuffer);
    } else {
        free(pth);
        res = (char *) malloc(strlen(msgBad) + 1);
        if (!res) {
            puts("Failed to malloc() in doGet()");
            return PTR_ERR;
        }
        strcpy(res, msgBad);
    }
    if (!res) {
        puts("doGet() failed");
    }
    return res;
}

static char *lsByAuthor(const char* dirPath, const char *author) {
    DIR *d;
    struct dirent *dir;
    char *res = (char *) malloc(1000);

    if (!res) {
        puts("malloc() in ls()");
        return PTR_ERR;
    }
    res[0] = '\0';
    d = opendir(dirPath);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
                continue;
            }
            if (dir->d_type != DT_DIR) {
                if (authorIsEqual(dirPath, dir->d_name, author)) {
                    strcat(res, dir->d_name);
                    strcat(res, "\n");
                }
            }
        }
        closedir(d);
    }
    return res;
}

static char *ls(const char* dirPath) {
    DIR *d;
    struct dirent *dir;
    char *res = (char *) malloc(1000);

    if (!res) {
        puts("malloc() in ls()");
        return PTR_ERR;
    }
    res[0] = '\0';
    d = opendir(dirPath);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
                continue;
            }
            if (dir->d_type == DT_DIR) {
                strcat(res, "section: ");
            } else {
                strcat(res, "article: ");
            }
            char* tmpStr = (char*)malloc(strlen(dir->d_name) + 2);
            strcpy(tmpStr, dir->d_name);
            strcat(tmpStr, "\n");
            strcat(res, tmpStr);
            free(tmpStr);
        }
        closedir(d);
    }
    return res;
}

int dirExists(const char* dirPath) {
    DIR *d = opendir(dirPath);
    if (d) {
        closedir(d);
        return 1;
    }
    return 0;
}

static int fileExists(const char* path) {
    FILE* file;
    if ((file = fopen(path, "r"))) {
        fclose(file);
        if (dirExists(path)) {
            return 0;
        }
        return 1;
    }
    return 0;
}

static int verifyCwd(const char *cwd) {
    char pth[CWD_MAX];

    strcpy(pth, root);
    strcat(pth, cwd);
    if (!dirExists(pth)) {
        puts("cwd dir not exists");
        return ERR;
    }
    if (strstr(cwd, "/.")) {
        puts("cwd contains \".\" character");
        return ERR;
    }
    if (!*cwd || cwd[strlen(cwd) - 1] != '/') {
        puts("cwd is empty or does not have \"/\" character on the end of line");
        return ERR;
    }
    return 0;
}

static char *makePath(const char *cwd) {
    char *pth;

    pth = (char *) malloc(strlen(cwd) + 1 + strlen(root));
    if (!pth) {
        return PTR_ERR;
    }
    strcpy(pth, root);
    strcat(pth, cwd);
    return pth;
}

