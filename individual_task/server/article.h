#ifndef ARTICLE_H
#define ARTICLE_H

int readArticle(const char* fileName, char** buffer,
		char** author, int* authorLen, char** content);
		
void releaseArticleBuffer(char** buffer);

#endif
