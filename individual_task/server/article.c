#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "article.h"

static bool readArticleInternal(const char* fileName, char** buffer);

static bool getAuthorAndContentInternal(const char* articleContent,
		char** author, int* authorLen, char** content);


// returns true on success
bool readArticle(const char* fileName, char** buffer,
		char** author, int* authorLen, char** content) {
	if (!readArticleInternal(fileName, buffer)) {
		return false;
	}
	if (!getAuthorAndContentInternal(*buffer, author, authorLen, content)) {
		releaseArticleBuffer(buffer);
		return false;
	}
	return true;
}

// returns true on success
static bool readArticleInternal(const char* fileName, char** buffer) {
    FILE* pFile = fopen(fileName, "rb");
	unsigned fileLen;
    if (pFile == NULL) {
		*buffer = NULL;
        return false;
    }
    fseek(pFile, 0, SEEK_END);
    fileLen = ftell(pFile);
    rewind(pFile);
    
    *buffer = (char*) malloc((fileLen+1)*sizeof(char));
    fread(*buffer, fileLen, 1, pFile);
	(*buffer)[fileLen] = '\0';
    fclose(pFile);
	return true;
}

// returns true on success
static bool getAuthorAndContentInternal(const char* articleContent,
		char** author, int* authorLen, char** content) {
	char* authorFieldEnd = (char*)articleContent;
	while (*authorFieldEnd && (*authorFieldEnd != '\n')) {
		authorFieldEnd++;
	}
	if (*authorFieldEnd == '\0') {
		return false;
	} else {
		if (author != NULL) {
			*author = (char*)articleContent;
		}
		if (authorLen != NULL) {
			*authorLen = authorFieldEnd - articleContent;
		}
		if (content != NULL) {
			*content = authorFieldEnd + 1;
		}
	}
	return true;
}

void releaseArticleBuffer(char** buffer) {
	if (*buffer != NULL) {
		free(*buffer);
		*buffer = NULL;
	}
}
