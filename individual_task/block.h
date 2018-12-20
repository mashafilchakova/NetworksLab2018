#ifndef BLOCK_H
#define BLOCK_H

typedef struct {
    int gramLen;
    void *gram;
} gram_wrapper_t;

gram_wrapper_t *gramWrapperInit(void *content, int blockSize);

int blockAdd(gram_wrapper_t *gramWrapper, const void *content, int blockSize);

int charBlockAdd(gram_wrapper_t *gramWrapper, const char *content);

void releaseGramWrapper(gram_wrapper_t *gramWrapper);

#endif

