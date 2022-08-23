#define _POSIX_C_SOURCE 200112L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include <boundedBuffer.h>
#include <list.h>
#include <hashtable.h>
#include <txtParser.h>
#include <locker.h>
#include <storage.h>
#include <functions.h>

#define BUFFERSIZE 256
#define UNIX_PATH_MAX 108
#define COMMLENGTH 1024

pthread_mutex_t mutexLog = PTHREAD_MUTEX_INITIALIZER;

#define LOG(...){ \
	if (pthread_mutex_lock(&mutexLog) != 0) { \
		perror("mutexLog lock"); \
		exit(EXIT_FAILURE); \
	} \
	fprintf(logFile, __VA_ARGS__); \
	if (pthread_mutex_unlock(&mutexLog) != 0) { \
		perror("mutexLog unlock"); \
		exit(EXIT_FAILURE); \
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

static void* signalHandling(void* set);

static void* workFunc(void* args);

int main (int argc, char** argv){

	if(argc != 2){
		printf("Usage: ./server '/path/to/config'\n");
		return 0;
	}
	time_t initTime = time(NULL);
	time_t currTime = time(NULL);
	double nowS = 0;
	int err;
	
	//signal handling
	struct sigaction s;
	sigset_t set;
	pthread_t sigHandler;
	memset(&s, 0, sizeof(s));
	s.sa_handler = SIG_IGN;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGHUP);
	s.sa_mask = set;
	if(sigaction(SIGPIPE, &s, NULL) != 0){
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	if((err = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0){
		errno = err;
		perror("pthread_sigmask");
		exit(EXIT_FAILURE);
	}
	if((err = pthread_create(&sigHandler, NULL, &signalHandling, (void*) &set)) != 0){
		errno = err;
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
	
	txtFile* config = txtInit();
	if(config == NULL){
		perror("Config file creation");
		exit(EXIT_FAILURE);
	}
	if((applyConfig(config, argv[1])) != 0){
		perror("Apply configuration");
		exit(EXIT_FAILURE);
	}
	
	//socket init
	char* socketPath = config->pathToSocket;
	int fd_skt = -1;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, socketPath, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;
	if((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	if((bind(fd_skt, (struct sockaddr*)&sa, sizeof(sa))) == -1){
		perror("bind");
		exit(EXIT_FAILURE);
	
	}
	if((listen(fd_skt, SOMAXCONN)) == -1){
		perror("listen");
		exit(EXIT_FAILURE);
	
	}
	
	//manager-worker pipe init
	int pipeMW[2];
	if (pipe(pipeMW) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);;
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
		exit(EXIT_FAILURE);
	}
	
	storage* storage;
	if((storage = storageInit(config->storageFileNumber, config->storageSize, config->repPolicy)) == NULL){
		perror("storageInit");
		exit(EXIT_FAILURE);
	}
	
	FILE* logFile = NULL;
	if((logFile = fopen(config->logPath, "w+")) == NULL){
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	
	//starting threads (to be fully implemented) 
	workArg* workArgs = malloc(sizeof(workArg));
	workArgs->initTime = initTime;
	workArgs->storage = storage;
	workArgs->commands = jobQueue;
	workArgs->pipeOut = pipeMW[1];
	workArgs->log = logFile;
	pthread_t* workerThreads;
	if((workerThreads = malloc(config->workerNumber * sizeof(pthread_t))) == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	int i = 0;
	while(i < config->workerNumber){
		
		if(((pthread_create(&(workerThreads[i]), NULL, &workFunc, (void*) workArgs))) != 0){
			perror("pthread_create workers");
			exit(EXIT_FAILURE);
		}
		i++;
	
	}
	
	//request management
	int clientsOnline = 0;
	fd_set setCopy;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;	//200 ms
	struct timeval timeCopy;
	int fd_client = -1;
	char command[COMMLENGTH];
	char pipeBuff[BUFFERSIZE];
	memset(command, 0, COMMLENGTH);
	int re_fd;
	
	while(true){
	
		//SIGINT or SIGQUIT
		if(term){
			goto cleanall;
		}
		//SIGHUP
		if(blockNewClients && clientsOnline == 0){
			goto cleanall;
		}
		
		setCopy = managerReadSet;
		timeCopy = timeout;
	
		if((select(fd_num+1, &setCopy, NULL, NULL, &timeCopy)) == -1){
		
			perror("select");
			goto cleanall;
		
		}
		i = 0;
		//we need to test all ready fds with FD_ISSET
		for(i = 0; i < fd_num + 1; i++){
			if(FD_ISSET(i, &setCopy)){
				//client connection
				if(i == fd_skt){
				
					if((fd_client = accept(fd_skt, NULL, 0)) != -1){					
						FD_SET(fd_client, &managerReadSet);
						clientsOnline++;
						if(fd_client > fd_num){
							fd_num = fd_client;
						}
						currTime = time(NULL);
						nowS = fabs(difftime(initTime, currTime));
						LOG("[%d] Server: new connection from client %d", (int) nowS, fd_client);
					}
					else{
						perror("accept");
						exit(EXIT_FAILURE);
					}
					
				}
				
				//pipe notifies that a request has been satisfied
				if(i == pipeMW[0]){
				
					if(readn(i, (void*) pipeBuff, BUFFERSIZE) <= 0){
						printf("Pipe error\n");
						exit(EXIT_FAILURE);
					}
					sscanf(pipeBuff, "%d", &re_fd);
					if(re_fd == -1){
						clientsOnline--;
						currTime = time(NULL);
						nowS = fabs(difftime(initTime, currTime));
						LOG("[%d] Server: client disconnected", (int) nowS);
						if(clientsOnline == 0 && blockNewClients){
							goto cleanall;
						}
					
					}
					else{
						FD_SET(re_fd, &managerReadSet);
						if(re_fd > fd_num){
							fd_num = re_fd;
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
					if(enqueueBuffer(jobQueue, command) == -1){
						perror("enqueue");
						exit(EXIT_FAILURE);
					
					}
				
				
				}
						
			}
		
		}
	}
	
	
	
	
	
	cleanall:
		int j = 0;
		snprintf(command, COMMLENGTH, "%d", -1);
		for (j = 0; j < config->workerNumber; j++){
			if(enqueueBuffer(jobQueue, command) != 0){
				perror("enqueue");
				exit(EXIT_FAILURE);
			}
		}
		for (j = 0; j < config->workerNumber; j++){
			pthread_join(workerThreads[j], NULL);
		}
		pthread_join(sigHandler, NULL);
		updateStorage(storage);
		printf("==STORAGE INFO==\n");
		printf("Max number of files stored: %d", storage->maxFileStored);
		printf("Max megabytes stored: %lu", (storage->maxMBStored)/1000000);
		printf("Replacement algorithm sent %d victims", storage->victimNumb);
		printf("Currently stored files:\n");
		printList(storage->filesFIFOQueue);
		LOG("Max megabytes stored: %lu\n", (storage->maxMBStored)/1000000);
		LOG("Max number of files stored: %d", storage->maxFileStored);
		LOG("Replacement algorithm sent %d victims", storage->victimNumb);
		unlink(socketPath);		
		if(cleanConf(config) != 0){
			perror("cleanConf");
			exit(EXIT_FAILURE);
		}
		freeStorage(storage);
		cleanBuffer(jobQueue);
		free(workerThreads);
		close(pipeMW[0]); 
		close(pipeMW[1]);
		close(fd_skt);
		LOG("Server terminated successfully\n");
		fclose(logFile);
		
	return 0;
}

static void* signalHandling(void* set){

	sigset_t* copySet = (sigset_t*) set;
	int sig;
	while(1){
		if((sigwait(copySet, &sig)) == 0){
			if(sig == SIGINT || sig == SIGQUIT){
				term = 1;
				return NULL;
			}
			if(sig == SIGHUP){
				blockNewClients = 1;
				return NULL;			
			}	
		
		}
	}


}

static void* workFunc(void* args){
	
	int err;
	workArg* arguments = (workArg*) args;
	time_t initTime = arguments->initTime;
	storage* storage = arguments->storage;
	boundedBuffer* commands = arguments->commands;
	int pOut = arguments->pipeOut;
	FILE* logFile = arguments->log;
	char pipeBuff[BUFFERSIZE];
	operation op;
	char* fdString;
	int fd_client;
	char* returnStr = NULL;
	char* command = NULL;
	char* tokComm;
	char* token;
	char* pathname;
	int flags;
	void* sentBuf;
	unsigned long sentSize; 
	unsigned long readSize;
	list* listOfFiles = NULL;
	elem* current;
	storedFile* tmp;
	int N;
	unsigned long writeSize;
	time_t currTime = time(NULL);
	double now = 0;
	char* writeFileContent;
	char sizeStr[BUFFERSIZE];
	char* strtokState = NULL;
	char* errorString = NULL;
	
	//messages from clients are like "opcode arguments"
	//strtok_r is used to retrieve the arguments
	while(1){
		fdString = NULL;
		strtokState = NULL;
		if(dequeueBuffer(commands, &fdString) != 0){
			perror("dequeue");
			exit(EXIT_FAILURE);		
		}
		sscanf(fdString, "%d", &fd_client);
		memset(command, 0, COMMLENGTH);
		if(readn(fd_client, (void*) command, COMMLENGTH) <= 0){
			exit(EXIT_FAILURE);		
		}
		tokComm = command;
		token = strtok_r(tokComm, " ", &strtokState);
		if(token != NULL){
			sscanf(token, "%d", (int*) &op);
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
						exit(EXIT_FAILURE);
					}
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: openFile %s %d exited with code: %d\n", (int) now, (int) pthread_self(), pathname, flags, err);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:			
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
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
						exit(EXIT_FAILURE);
					}
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: readFile %s exited with code: %d. Read size: %lu\n", (int) now, (int) pthread_self(), pathname, err, sentSize);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%lu", sentSize);
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						exit(EXIT_FAILURE);
					}
					if(sentSize != 0){
						if((writen(fd_client, (void*) sentBuf, sentSize)) <= 0){
							exit(EXIT_FAILURE);
						}
					}
					free(sentBuf);
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;			
					
				//READN
				case READN: 
					listOfFiles = NULL;
					N = 0;
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%d", &N);
					err = storageReadNFiles(storage, N, &listOfFiles, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: readNFile %d exited with code: %d\n", (int) now, (int) pthread_self(), N, err);
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						exit(EXIT_FAILURE);
					}
					int nFiles = elemsNumber(listOfFiles);
					int j = nFiles;
					current = getHead(listOfFiles);
					tmp = current->info;
					readSize = 0;
					while(j > 0){
						readSize = readSize + tmp->size;
						memset(returnStr, 0, BUFFERSIZE);
						snprintf(returnStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) returnStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%lu", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						if(tmp->size != 0){
							if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
								exit(EXIT_FAILURE);
							}
						}
						current = current->next;
						tmp = current->info;
						j--;
					
					}
					freeList(listOfFiles);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: readNFile :: Number of read files: %d. Read size: %lu\n", (int) now, (int) pthread_self(), nFiles, readSize);
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;			
					
				//WRITE
				case WRITE: 
					listOfFiles = NULL;
					writeFileContent= NULL;
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, COMMLENGTH);
					sscanf(token, "%s", pathname);
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%lu", &writeSize);
					if(writeSize != 0){
						if((writeFileContent = malloc(writeSize * sizeof(char))) == NULL){
							exit(EXIT_FAILURE);
						} 
						memset(writeFileContent, 0, writeSize+1);
						if((readn(fd_client, (void*) writeFileContent, writeSize)) <= 0){
							exit(EXIT_FAILURE);		
						}
					
					}
					err = storageWriteFile(storage, pathname, writeFileContent, writeSize, &listOfFiles, fd_client);
					free(writeFileContent);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: writeFile %s exited with code: %d\n. Write size: %lu\n", (int) now, (int) pthread_self(), pathname, err, writeSize);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						exit(EXIT_FAILURE);
					}
					int n = elemsNumber(listOfFiles);
					current = getHead(listOfFiles);
					tmp = current->info;
					while(n > 0){
						memset(returnStr, 0, BUFFERSIZE);
						snprintf(returnStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) returnStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%lu", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
							exit(EXIT_FAILURE);
						}
						currTime = time(NULL);
						now = fabs(difftime(initTime, currTime));
						LOG("[%d] Thread %d: writeFile %s: a victim has been chosen. Victim name: %s Victim size: %lu\n", (int) now, (int) pthread_self(), pathname, tmp->name, tmp->size);
						current = current->next;
						tmp = current->info;
						n--;
					
					}
					freeList(listOfFiles);
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;			

				//APPEND
				case APPEND:
					listOfFiles = NULL;
					writeFileContent = NULL;
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, COMMLENGTH);
					sscanf(token, "%s", pathname);
					token = strtok_r(NULL, " ", &strtokState);
					sscanf(token, "%lu", &writeSize);
					if(writeSize != 0){
						if((writeFileContent = malloc(writeSize * sizeof(char))) == NULL){
							exit(EXIT_FAILURE);
						} 
						memset(writeFileContent, 0, writeSize+1);
						if((readn(fd_client, (void*) writeFileContent, writeSize)) <= 0){
							exit(EXIT_FAILURE);		
						}
					
					}
					err = storageAppendFile(storage, pathname, writeFileContent, writeSize, &listOfFiles, fd_client);
					free(writeFileContent);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: appendToFile %s exited with code: %d\n. Write size: %lu\n", (int) now, (int) pthread_self(), pathname, err, writeSize);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(sizeStr, 0, BUFFERSIZE);
					snprintf(sizeStr, BUFFERSIZE, "%d", elemsNumber(listOfFiles));
					if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
						exit(EXIT_FAILURE);
					}
					int a = elemsNumber(listOfFiles);
					current = getHead(listOfFiles);
					tmp = current->info;
					while(a > 0){
						memset(returnStr, 0, BUFFERSIZE);
						snprintf(returnStr, BUFFERSIZE, "%s", tmp->name);
						if((writen(fd_client, (void*) returnStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						memset(sizeStr, 0, BUFFERSIZE);
						snprintf(sizeStr, BUFFERSIZE, "%lu", tmp->size);
						if((writen(fd_client, (void*) sizeStr, BUFFERSIZE)) <= 0){
							exit(EXIT_FAILURE);
						}
						if((writen(fd_client, (void*) tmp->content, tmp->size)) <= 0){
							exit(EXIT_FAILURE);
						}
						currTime = time(NULL);
						now = fabs(difftime(initTime, currTime));
						LOG("[%d] Thread %d: appendToFile %s: a victim has been chosen. Victim name: %s Victim size: %lu\n", (int) now, (int) pthread_self(), pathname, tmp->name, tmp->size);
						current = current->next;
						tmp = current->info;
						a--;
					
					}
					freeList(listOfFiles);
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;
				//LOCK	
				case LOCK:
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, COMMLENGTH);
					sscanf(token, "%s", pathname);
					err = storageLockFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: lockFile %s exited with code: %d\n", (int) now, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch (err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;
				//UNLOCK	
				case UNLOCK:
					token = strtok_r(NULL, " ", &strtokState);
					memset(pathname, 0, COMMLENGTH);
					sscanf(token, "%s", pathname);
					err = storageUnlockFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: unlockFile %s exited with code: %d\n", (int) now, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch (err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void*) errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
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
					err = storageCloseFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: closeFile %s exited with code: %d\n", (int) now, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
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
					err = storageCloseFile(storage, pathname, fd_client);
					memset(returnStr, 0, 32);
					snprintf(returnStr, 32, "%d", err);
					currTime = time(NULL);
					now = fabs(difftime(initTime, currTime));
					LOG("[%d] Thread %d: removeFile %s exited with code: %d\n", (int) now, (int) pthread_self(), pathname, err);
					if((writen(fd_client, (void*) returnStr, strlen(returnStr) + 1)) <= 0){
						exit(EXIT_FAILURE);
					}
					switch(err){
						case 0: 
							break;
						
						case -1:
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							break;
							
						case -2:				
							memset(errorString, 0, 32);
							snprintf(errorString, 32, "%d", errno);
							if(writen(fd_client, (void *)errorString, 32) == -1){
								exit(EXIT_FAILURE);
							}
							exit(EXIT_FAILURE);
					
					}
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", fd_client);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;
					
				//CLOSECONN
				case CLOSECONN:
					memset(pipeBuff, 0, BUFFERSIZE);
					snprintf(pipeBuff, BUFFERSIZE, "%d", -1);
					if(writen(pOut, (void*) pipeBuff, BUFFERSIZE) == -1){
						exit(EXIT_FAILURE);
					}
					break;		
										
			}
			
		}
		free(fdString);
	
	}
	return 0;
}
