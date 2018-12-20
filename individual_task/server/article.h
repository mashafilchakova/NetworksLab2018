bool readArticle(const char* fileName, char** buffer,
		char** author, int* authorLen, char** content);
		
void releaseArticleBuffer(char** buffer);
