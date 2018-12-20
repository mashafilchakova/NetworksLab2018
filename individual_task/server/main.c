// socket server example, handles multiple clients using threads
#include <stdbool.h>
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <dirent.h> 
#include "article.h"

#define MAX_BUFFER_SIZE 256
#define MAX_PATH_SIZE 512

char rootDir[MAX_PATH_SIZE];

//the thread function
void *connection_handler(void *);

bool dirExists(const char* dirPath);
bool fileExists(const char* path);

int main(int argc , char *argv[])
{
	if (argc == 1) {
		strcpy(rootDir, "data/");
	} else if (argc == 2) {
		strcpy(rootDir, argv[1]);
		if (!dirExists(rootDir)) {
			printf("Directory %s does not exist\n", rootDir);
			return 1;
		}
	} else {
		printf("Use \"%s <rootDirForArticles>\"\n", argv[0]);
		printf("Or use \"%s\", if you want to use data/ dir as root\n", argv[0]);
		return 1;
	}
	int rootDirLen = strlen(rootDir);
	if (rootDir[rootDirLen - 1] != '/') {
		if (rootDir[rootDirLen - 1] == '\\') {
			rootDir[rootDirLen - 1] = '/';
		} else {
			strcat(rootDir, "/");
		}
	}
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 3000 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    c=sizeof(struct sockaddr_in);
    while ((client_sock = accept(socket_desc,(struct sockaddr*)&client,(socklen_t*)&c)))
    {
        puts("Connection accepted");

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        puts("Handler assigned");
    }

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

void ls(const char* dirPath, int sock) {
	DIR *d;
	struct dirent *dir;
	d = opendir(dirPath);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
				continue;
			}
			if (dir->d_type == DT_DIR) {
				send(sock,"section: ",strlen("section: "),0);
			} else {
				send(sock,"article: ",strlen("article: "),0);
			}
			char* tmpStr = (char*)malloc(sizeof(char) * (strlen(dir->d_name) + 1));
			strcpy(tmpStr, dir->d_name);
			strcat(tmpStr, "\n");
			send(sock,tmpStr,strlen(tmpStr),0);
			free(tmpStr);
		}
		closedir(d);
	}
}

bool dirExists(const char* dirPath) {
	DIR *d = opendir(dirPath);
	if (d) {
		closedir(d);
		return true;
	}
	return false;
}

bool fileExists(const char* path) {
	FILE* file;
	if ((file = fopen(path, "r"))) {
		fclose(file);
		if (dirExists(path)) {
			return false;
		}
		return true;
	}
	return false;
}

void cd(int sock, char* curDir, char* cmd) {
	char nextDir[MAX_PATH_SIZE];
	char* sectionName = cmd + strlen("cd ");
	bool success = true;
	if (!strcmp(sectionName, ".") || !strcmp(sectionName, "")) {
	} else if (!strcmp(sectionName, "..")) {
		if (!strcmp(curDir, rootDir)) {
			success = false;
		} else {
			// go to upper section
			char* endOfDir = curDir + (strlen(curDir) - 2);
			while (*endOfDir != '/') {
				endOfDir--;
			}
			endOfDir++;
			*endOfDir = '\0';
		}
	} else if (!strstr(sectionName, "/") && !strstr(sectionName, "\\")) {
		// go to section
		strcpy(nextDir, curDir);
		strcat(nextDir, sectionName);
		strcat(nextDir, "/");
		if (dirExists(nextDir)) {
			strcpy(curDir, nextDir);
		} else {
			success = false;
		}
	} else {
		success = false;
	}
	if (success) {
		const char* curDirForClient = curDir+(strlen(rootDir) - 1);
		send(sock, curDirForClient, strlen(curDirForClient), 0);
		send(sock, "\n", 1, 0);
	} else {
		const char* noSectionMsg = "There is no such section\n";
		send(sock, noSectionMsg, strlen(noSectionMsg), 0); 
	}
}

void get(int sock, char* curDir, char* cmd) {
	char path[MAX_PATH_SIZE];
	char* articleName = cmd + strlen("get ");
	if (!strstr(articleName, "/") && !strstr(articleName, "\\")) {
		strcpy(path, curDir);
		strcat(path, articleName);
		if (!fileExists(path)) {
			const char* notExistMsg = "Article does not exist\n";
			send(sock, notExistMsg, strlen(notExistMsg), 0); 
			return;
		}
	}
	char* articleBuffer;
	char* author;
	int authorLen;
	char* articleContent;
	if (!readArticle(path, &articleBuffer, &author, &authorLen, &articleContent)) {
		const char* badArticleMsg = "System: Sorry, the article is corrupted\n";
		send(sock, badArticleMsg, strlen(badArticleMsg), 0);
		return;
	}
	const char* articleTitleMsg = "Title: ";
	send(sock, articleTitleMsg, strlen(articleTitleMsg), 0);
	send(sock, articleName, strlen(articleName), 0);
	const char* authorMsg = "\nAuthor: ";
	send(sock, authorMsg, strlen(authorMsg), 0);
	send(sock, author, authorLen, 0);
	const char* articleTextMsg = "\nArticle content:\n";
	send(sock, articleTextMsg, strlen(articleTextMsg), 0);
	send(sock, articleContent, strlen(articleContent), 0);
	send(sock, "\n", 1, 0);
	releaseArticleBuffer(&articleBuffer);
	return;
}

void getByAuthor(int sock, char* curDir, char* cmd) {
	char path[MAX_PATH_SIZE];
	char* requiredAuthor = cmd + strlen("getByAuthor ");
	
	DIR *d;
	struct dirent *dir;
	d = opendir(curDir);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
				continue;
			}
			if (dir->d_type != DT_DIR) {
				strcpy(path, curDir);
				strcat(path, dir->d_name);
				char* articleBuffer;
				char* author;
				int authorLen;
				if (readArticle(path, &articleBuffer, &author, &authorLen, NULL)) {
					if (authorLen == strlen(requiredAuthor)) {
						if (!strncmp(author, requiredAuthor, authorLen)) {
							send(sock, dir->d_name, strlen(dir->d_name), 0);
							send(sock, "\n", 1, 0);
						}
					}
					releaseArticleBuffer(&articleBuffer);
				}
			}
		}
		closedir(d);
	}
}

// returns true on success
bool getBlock(int sock, char** buf) {
	*buf = NULL;
	uint64_t len;
	if (recv(sock, &len, sizeof(uint64_t), 0) != sizeof(uint64_t)) {
		return false;
	}
	char* buffer = (char*)malloc(len + 1);
	if (!buffer) {
		return false;
	}
	if (recv(sock, buffer, len, 0) != len) {
		free(buffer);
		return false;
	}
	buffer[len] = '\0';
	*buf = buffer;
	return true;
}

void releaseBlock(char** buffer) {
	if (*buffer != NULL) {
		free(*buffer);
		*buffer = NULL;
	}
}

void put(int sock, const char* curDir, const char* cmd) {
	char *title, *author, *content;
	if (!getBlock(sock, &title)) {
		return;
	}
	if (!getBlock(sock, &author)) {
		releaseBlock(&title);
		return;
	}
	char *path = (char*) malloc(strlen(curDir) + strlen(title) + 1);
	strcpy(path, curDir);
	strcat(path, title);
	FILE* f;
	bool success = false;
	if (!strstr(title, "/") && !strstr(title, "\\") &&
			!fileExists(path) && !dirExists(path)) {
		if ((f = fopen(path, "wb")) > 0) {
			if (fwrite(author, sizeof(char), strlen(author), f) > 0) {
				if (fwrite("\n", sizeof(char), 1, f) > 0) {
					while(1) {
						if (!getBlock(sock, &content)) {
							break;
						}
						if (!strcmp(content, "\n")) {
							success = true;
							releaseBlock(&content);
							break;
						}
						if (fwrite(content, sizeof(char), strlen(content), f) <= 0) {
							releaseBlock(&content);
							break;
						}
						releaseBlock(&content);
					}
				}
			}
			fclose(f);
		}
	}
	if (success) {
		const char* successMsg = "Article successfully added to the section\n";
		send(sock, successMsg, strlen(successMsg), 0);
	} else {
		while (1) {
			if (!getBlock(sock, &content)) {
				break;
			}
			if (!strcmp(content, "\n")) {
				releaseBlock(&content);
				break;
			}
			releaseBlock(&content);
		}
		const char* failedMsg = "Failed to add an article\n";
		send(sock, failedMsg, strlen(failedMsg), 0);
	}
	releaseBlock(&title);
	releaseBlock(&author);
	free(path);
}

/*
  This will handle connection for each client
  */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int n;

	char cmd[MAX_BUFFER_SIZE];

	char curDir[MAX_PATH_SIZE];
	strcpy(curDir, rootDir);
	
	while(1) {
		if ((n=recv(sock,cmd,MAX_BUFFER_SIZE - 1,0))>0) {
			//printf("cmd = %s\n", cmd);
			cmd[n] = '\0';
			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}
			if (!strcmp(cmd, "quit")) {
				n = 0;
				break;
			} else if (!strcmp(cmd, "ls")) {
				ls(curDir, sock);
			} else if (strstr(cmd, "cd ") == cmd) {
				cd(sock, curDir, cmd);
			} else if (strstr(cmd, "get ") == cmd) {
				get(sock, curDir, cmd);
			} else if (strstr(cmd, "getByAuthor ") == cmd) {
				getByAuthor(sock, curDir, cmd);
			} else if (!strcmp(cmd, "put")) {
				put(sock, curDir, cmd);
			} else {
				const char* noCmdMsg = "No command\n";
				send(sock, noCmdMsg, strlen(noCmdMsg), 0);
			}
			send(sock, "\0\n", 2, 0);
		}
		if (n <= 0) {
			break;
		}
	}
	close(sock);

	if(n==0) {
		puts("Client Disconnected");
	} else {
		perror("recv failed");
	}
	return 0;
}