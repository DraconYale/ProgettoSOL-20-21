#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#include <boundedBuffer.h>
#include <list.h>
#include <txtParser.h>
#include <functions.h>
#include <storage.h>

#define BUFFERSIZE 256
#define UNIX_PATH_MAX 108
#define COMMLENGTH 1024

#define LOG(...){ \
	if (pthread_mutex_lock(&mutexLog) != 0) { \
		perror("mutexLog lock"); \
		return -1; \
	} \
	fprintf(logFile, __VA_ARGS__); \
	if (pthread_mutex_unlock(&mutexLog) != 0) { \
		perror("mutexLog unlock"); \
		return -1; \
	} \
}

typedef struct workArg{
	time_t initTime;
	storage* storage;
	boundedBuffer* commands;
	int pipeOut;
	FILE* log;
}workArg;

volatile sig_atomic_t term = 0; 		//term = 1 when SIGINT or SIGQUIT
volatile sig_atomic_t blockNewClients = 0; 	//blockNewClients = 1 when SIGHUP

void* signalHandling(void* set){

	sigset_t* copySet = (sigset_t*) set;
	int sig;
	while(1){
		if((sigwait(copySet, &sig)) == 0){
			if(sig == SIGINT || sig == SIGQUIT){
				term = 1;
			}
			if(sig == SIGHUP){
				blockNewClients = 1;			
			}	
		
		}
	}


}

void* workFunc(void* args){
	
	int err;
	workArg* arguments = (workArg*) args;
	time_t initTime = arguments->initTime;
	storage* storage = arguments->storage;
	boundedBuffer* commands = arguments->commands;
	int pOut = arguments->pipeOut;
	FILE* logFile = arguments->log;
	operation op;
	char* fdString;
	int fd_client;
	char* returnStr;
	char* command;
	char* tokComm;
	char* token;
	char* operation;
	char* pathname;
	int flags;
	void* sentBuf;
	long sentSize; 
	long readSize;
	char* readFileContent;
	list** listOfFiles = NULL;
	int N;
	long writeSize;
	time_t currTime = time(NULL);
	double time = 0;
	char* writeFileContent;
	char* sizeStr[BUFFERSIZE];
	char* strtokState = NULL;
	char* errorString;
	
	//messages from clients are like "opcode arguments"
	//strtok_r is used to retrieve the arguments
	while(1){
		fdString = NULL;
		strtokState = NULL;
		if(bufferDequeue(commands, &fdString) != 0){
			perror("dequeue");
			return -1;		
		}
		sscanf(fdString, "%d", &fd_client);
		memset(command, 0, COMMLENGTH);
		if(readn(fd_ready, (void*) command, COMMLENGTH)) <= 0){
			return -1;		
		}
		token = strtok_r(tokComm, " ", &strtokState);
		if(token != NULL){
			sscanf(token, "%d", &op);
			
			switch(op){
			
				//OPEN
				case OPEN: 
					memset(pathname, 0, UNIX_PATH_MAX);
					token = strtok_r(NULL, " ", &strtokState);
					pathname = token;
					flags = 0;
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%d", &flags);
					err = storageOpenFile(storage, pathname, flags, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: openFile %s %d exited with code: %d\n", (int) time, (int) pthread_self(), pathname, flags, err);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;			
				
				//READ
				case READ: 
					sentBuf = NULL;
					sentSize = 0;
					memset(pathname, 0, UNIX_PATH_MAX);
					token = strtok_r(NULL, " ", &strtokState);
					pathname = token;
					err = storageReadFile(storage, pathname, &sentBuf, &sentSize, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: readFile %s exited with code: %d. Read size: %l\n", (int) time, (int) pthread_self(), pathname, err, sentSize);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%l", sentSize);
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						return -1;
					}
					if(sentSize != 0){
						if((writen(fd_client, (void*) sentBuf, sentSize) <= 0){
							return -1;
						}
					}
					free(sentBuf);
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;			
					
				//READN
				case READN: 
					listOfFiles = NULL;
					N = 0;
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%d", &N);
					err = storageReadNFile(storage, N, &listOfFiles, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: readNFile %d exited with code: %d\n", (int) time, (int) pthread_self(), N, err);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						return -1;
					}
					int nFiles = elemsNumber(listOfFiles);
					int j = nFiles;
					list* current = getHead(listOfFiles);
					storedFile* tmp = current->info;
					readSize = 0;
					while(j > 0){
						readSize = readSize + tmp->size;
						memset(retStr, 0, BUFFERSIZE);
						snprintf(retStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) retStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%l", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						if(tmp->size != 0){
							if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
								return -1;
							}
						}
						current = current->next;
						tmp = current->info;
						j--;
					
					}
					freeList(listOfElem);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: readNFile :: Number of read files: %d. Read size: %l\n", (int) time, (int) pthread_self(), nFiles, readSize);
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;			
					
				//WRITE
				case WRITE: 
					listOfFiles = NULL;
					writeContent = NULL;
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, REQUESTLEN);
					sscanf(token, "%s", pathname);
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%l", writeSize);
					if(writeSize != 0){
						if((writeFileContent = malloc(writeSize * sizeof(char))) == NULL{
							return -1
						} 
						memset(writeContent, 0, writeSize+1);
						if(readn(fd_ready, (void*) writeFileContent, writeSize)) <= 0){
							return -1;		
						}
					
					}
					err = storageWriteFile(storage, pathname, writeFileContent, writeSize, &listOfFiles, fd_client);
					free(writeContents);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: writeFile %s exited with code: %d\n. Write size: %l\n", (int) time, (int) pthread_self(), pathname, err, writeSize);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						return -1;
					}
					int j = elemsNumber(listOfFiles);
					list* current = getHead(listOfFiles);
					storedFile* tmp = current->info;
					while(j > 0){
						memset(retStr, 0, BUFFERSIZE);
						snprintf(retStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) retStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%l", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
							return -1;
						}
						currTime = time(NULL);
						time = fabs(difftime(initTime, currTime);
						LOG("[%d] Thread %d: writeFile %s: a victim has been chosen. Victim name: %s Victim size: %l\n", (int) time, (int) pthread_self(), pathname, tmp->name, tmp->size);
						current = current->next;
						tmp = current->info;
						j--;
					
					}
					freeList(listOfFiles);
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;			

				//APPEND
				case APPEND:
					listOfFiles = NULL;
					writeContent = NULL;
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, REQUESTLEN);
					sscanf(token, "%s", pathname);
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%l", writeSize);
					if(writeSize != 0){
						if((writeContent = malloc(writeSize * sizeof(char))) == NULL{
							return -1
						} 
						memset(writeContent, 0, writeSize+1);
						if(readn(fd_ready, (void*) writeContent, writeSize)) <= 0){
							return -1;		
						}
					
					}
					err = storageAppendFile(storage, pathname, writeFileContent, writeSize, &listOfFiles, fd_client);
					free(writeContents);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: appendToFile %s exited with code: %d\n. Write size: %l\n", (int) time, (int) pthread_self(), pathname, err, writeSize);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						return -1;
					}
					int j = elemsNumber(listOfFiles);
					list* current = getHead(listOfFiles);
					storedFile* tmp = current->info;
					while(j > 0){
						memset(retStr, 0, BUFFERSIZE);
						snprintf(retStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) retStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%l", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							return -1;
						}
						if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
							return -1;
						}
						currTime = time(NULL);
						time = fabs(difftime(initTime, currTime);
						LOG("[%d] Thread %d: appendToFile %s: a victim has been chosen. Victim name: %s Victim size: %l\n", (int) time, (int) pthread_self(), pathname, tmp->name, tmp->size);
						current = current->next;
						tmp = current->info;
						j--;
					
					}
					freeList(listOfElem);
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
				//LOCK	
				case LOCK:
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, REQUESTLEN);
					sscanf(token, "%s", pathname);
					err = storageLockFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: lockFile %s exited with code: %d\n", (int) time, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch (err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
				//UNLOCK	
				case UNLOCK:
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, REQUESTLEN);
					sscanf(token, "%s", pathname);
					err = storageUnlockFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: unlockFile %s exited with code: %d\n", (int) time, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch (err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
				//CLOSE	
				case CLOSE: 
					memset(pathname, 0, UNIX_PATH_MAX);
					token = strtok_r(NULL, " ", &strtokState);
					pathname = token;
					flags = 0;
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%d", &flags);
					err = storageCloseFile(storage, pathname, flags, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: closeFile %s exited with code: %d\n", (int) time, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
				//REMOVE	
				case REMOVE: 
					memset(pathname, 0, UNIX_PATH_MAX);
					token = strtok_r(NULL, " ", &strtokState);
					pathname = token;
					flags = 0;
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%d", &flags);
					err = storageCloseFile(storage, pathname, flags, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					time = fabs(difftime(initTime, currTime);
					LOG("[%d] Thread %d: removeFile %s exited with code: %d\n", (int) time, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						return -1;
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							break;
							
						case -2				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								return -1;
							}
							return -1;
					
					}
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
					
				//CLOSECONN
				case CLOSECONN:
					memset(pOut, 0, BUFFERSIZE);
					snprintf(pOut, BUFFERSIZE, "%d", -1);
					if(writen(pOut, (void*) pOut, BUFFERSIZE) == -1){
						return -1;
					}
					break;
					
										
			}
			free(fdString);
	}
	free(op);
	return NULL;
}

int main (int argc, char** argv){

	if(argc != 2){
		printf("Usage: ./server '/path/to/config'\n");
		return 0;
	}
	time_t initTime = time(NULL);
	time_t currTime = time(NULL);
	int err;
	
	//signal handling
	struct sigaction s;
	sigset_t set;
	pthread_t sigHandler;
	memset(&s, 0, sizeof(s));
	s->sa_handler = SIG_IGN;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGHUP);
	s->sa_mask = set;
	if(sigaction(SIGPIPE, &s, NULL) != 0){
		perror("sigaction");
		return -1;
	}
	if((err = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0){
		errno = err;
		perror("pthread_sigmask");
		return -1;
	}
	if((err = pthread_create(&sigHandler, NULL, &signalHandling, (void*) &set)) != 0){
		errno = err;
		perror("pthread_create");
		return -1;
	}
	
	//socket init
	char* socket = config->pathToSocket;
	int fd_skt = -1;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, socket, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;
	if((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket");
		return -1;
	}
	
	if((bind(fd_skt, (struct sockaddr*)&saddr, sizeof saddr)) == -1){
		perror("bind");
		return -1;
	
	}
	if((listen(fd_skt, SOMAXCONN)) == -1){
		perror("listen");
		return -1;
	
	}
	
	//manager-worker pipe init
	int pipeMW[2];
	if (pipe(pipeMW) == -1){
		perror("pipe");
		return -1;;
	}
	
	//managerReadSet init (used for select SC)
	int fd_num = 0;
	if(fd_skt > fd_num){
		fd_num = fd_skt;
	}
	fd_set managerReadSet;
	FD_ZERO(&managerReadSet);
	FD_SET(fd_skt, &managerReadSet);
	FD_SET(pipeMW[0], &managerReadSet);
	
	//structures init
	boundedBuffer* jobQueue = initBuffer(BUFFERSIZE);
	if(jobQueue == NULL){
		perror("Bounded Buffer creation");
		return -1;
	}
	
	txtFile* config = txtInit();
	if(config == NULL){
		perror("Config file creation");
		return -1;
	}
	if((applyConfig(config, argv[1])) != 0){
		perror("Apply configuration");
		return -1;
	}
	
	storage* storage;
	if((storage = storageInit(config->storageFilesNumber, config->storageSize, config->repPolicy)) == NULL){
		perror("storageInit");
		return -1;
	}
	
	FILE* logFile = NULL;
	pthread_mutex_t mutexLog = PTHREAD_MUTEX_INITIALIZER;
	if((logFile = fopen(config->logPath, w+)) == NULL){
		perror("fopen");
		return -1;
	}
	
	//starting threads (to be fully implemented) 
	workArg* workArgs = malloc(sizeof(workArg));
	workArgs->initTime = initTime;
	workArgs->storage = storage;
	workArgs->commands = jobQueue;
	workArgs->pipeOut = pipeMW[1];
	workArgs->log = logFile;
	pthred_t* workerThreads;
	if((workerThreads = malloc(config->workerNumber * sizeof(pthread_t))) == NULL){
		perror("malloc");
		return -1;
	}
	int i = 0;
	while(i < config->workerNumber){
		
		if((pthread_create(&(workerThreads[i], NULL, &workerFunc, (void*) workArgs))) != 0){
			perror("pthread_create workers");
			return -1;
		}
		i++;
	
	}
	
	//request management
	int clientsOnline = 0;
	fd_set setCopy;
	struct timeval timeout;
	timeout->tv_sec = 0;
	timeout->tv_usec = 200000	//200 ms
	struct timeval timeCopy;
	int fd_client = -1;
	char* command[COMMLENGTH];
	memset(command, 0, COMMLENGTH);
	int re_fd;
	
	while(true){
	
		//SIGINT or SIGQUIT
		if(term){
			goto cleanall
		}
		//SIGHUP
		if(blockNewClients && clientsOnline == 0){
			goto cleanall
		}
		
		setCopy = readManagerSet;
		timeCopy = timeout;
	
		if((select(fd_num+1, &setCopy, NULL, NULL, &timeCopy)) == -1){
		
			perror("select")
			goto cleanall
		
		}
		i = 0;
		//we need to test all ready fds with FD_ISSET
		for(i = 0, i < fd_num + 1; i++){
			if(FD_ISSET(i, &setCopy)){
				//client connection
				if(i == fd_skt){
				
					if((fd_client = accept(fd_skt, NULL, 0)) != -1){					
						FD_SET(fd_client, &readManagerSet);
						clientsOnline++;
						if(fd_client > fd_num){
							fd_num = fd_client;
						}
						currTime = time(NULL);
						time = fabs(difftime(initTime, currTime);
						LOG("[%d] Server: new connection from client %d", (int) time, fd_client);
					}
					else{
						perror("accept");
						return -1;
					}
					
				}
				
				//pipe notifies that a request has been satisfied
				if(i == pipeMW[0]){
				
					if(readn(i, (void*) re_fd, sizeof(int)) <= 0){
						printf("Pipe error\n");
						return -1;
					}
					if(re_fd == -1){
						clientsOnline--;
						currTime = time(NULL);
						time = fabs(difftime(initTime, currTime);
						LOG("[%d] Server: client disconnected", (int) time);
						if(clientsOnline == 0 && blockNewClients){
							goto cleanall
						}
					
					}
					else{
						FD_SET(re_fd, &managerReadSet);
						if(re_fd > fd_num){
							fd_num = re_fd,
						}
					}
									
				}
				//new request
				else{
					memset(command, 0, COMMLENGTH);
					snprintf(command, COMMLENGTH, "%d", i);
					if(i == fd_num){
						fd_num--;					
					}
					if(bufferEnqueue(jobQueue, command) == -1){
						perror("enqueue");
						return -1;
					
					}
				
				
				}
						
			}
		
		}
	}
	
	
	
	
	
	cleanall:
		int j = 0;
		snprintf(command, COMMLENGTH, "%d", -1);
		for (j = 0; j < config->workerNumber; j++){
			if(BoundedBuffer_Enqueue(jobQueue, command) != 0){
				perror("enqueue");
				return -1;
			}
		}
		for (j = 0; j < config->workerNumber; j++){
			pthread_join(workerThreads[j], NULL);
		}
		pthread_join(sigHandler, NULL);
		updateStorage(storage);
		printf("==STORAGE INFO==\n")
		printf("Max number of files stored: %d", storage->maxFileStored);
		printf("Max megabytes stored: %l", (storage->maxMBStored)/1000000);
		printf("Replacement algorithm sent %d victims", storage->victimNumb);
		printf("Currently stored files:\n");
		printList(storage->filesFIFOQueue);
		LOG("Max megabytes stored: %l\n", (storage->maxMBStored)/1000000);
		LOG("Max number of files stored: %d\n", Storage_GetReachedFiles(storage));
		LOG("Replacement algorithm sent %d victims", storage->victimNumb);		
		if(cleanConf(config) != 0){
			perror("cleanConf");
			return -1;
		}
		freeStorage(storage);
		cleanBuffer(jobQueue);
		free(workerThreads);
		unlink(socket);
		close(pipeMW[0]); 
		close(pipeMW[1]);
		close(fd_skt);
		LOG("Server terminated successfully\n");
		fclose(logFile);
		
	return 0;
}
