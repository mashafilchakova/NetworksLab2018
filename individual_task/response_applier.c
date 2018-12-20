#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "response_applier.h"
#include "cmd_common.h"

#define ERR (-1)
#define PTR_ERR NULL

extern char curDir[];

// return -1 on error; otherwise, 0
int applyResponse(const void *responseGram, int responseGramLen) {
    char *curSecMsg = "Current section is changed to: ";
    if (strstr(responseGram, curSecMsg) == responseGram) {
        strcpy(curDir, ((const char *) responseGram) + strlen(curSecMsg));
        if (curDir[strlen(curDir) - 1] == '\n') {
            curDir[strlen(curDir) - 1] = '\0';
        }
    }
    printf("%s", (char *) responseGram);
    return 0;
}

