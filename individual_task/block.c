#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "block.h"

#define PTR_ERR NULL
#define ERR (-1)

gram_wrapper_t *gramWrapperInit(void *content, int blockSize) {
    gram_wrapper_t *gramWrapper;
    void *gram;

    gram = malloc(sizeof(blockSize));
    if (!gram) {
        return PTR_ERR;
    }
    gramWrapper = (gram_wrapper_t *) malloc(sizeof(gram_wrapper_t));
    if (!gramWrapper) {
        free(gram);
        return PTR_ERR;
    }
    gramWrapper->gramLen = blockSize;
    gramWrapper->gram = gram;
    memcpy(gramWrapper->gram, content, blockSize);
    return gramWrapper;
}

// return -1 on error;
//        0 on success;
// if error was occured, then you don't need to release block;
// also block pointer became NULL for you
int blockAdd(gram_wrapper_t *gramWrapper, const void *content, int blockSize) {
    void *gram;

    gram = realloc(gramWrapper->gram, gramWrapper->gramLen + blockSize);
    if (!gram) {
        free(gramWrapper->gram);
        free(gramWrapper);
        return ERR;
    }
    gramWrapper->gram = gram;
    memcpy(gramWrapper->gram + gramWrapper->gramLen, content, blockSize);
    gramWrapper->gramLen += blockSize;
    return 0;
}

int charBlockAdd(gram_wrapper_t *gramWrapper, const char *content) {
    void *block;
    uint32_t contentLen = strlen(content) + 1;
    int blockLen = sizeof(uint32_t) + contentLen;
    int res;

    block = malloc(blockLen);
    if (!block) {
        free(gramWrapper->gram);
        free(gramWrapper);
        return ERR;
    }
    memcpy(block, &contentLen, sizeof(uint32_t));
    memcpy(block + sizeof(uint32_t), content, contentLen);
    res = blockAdd(gramWrapper, block, blockLen);
    free(block);
    return res;
}

void releaseGramWrapper(gram_wrapper_t *gramWrapper) {
    free(gramWrapper->gram);
    free(gramWrapper);
}














