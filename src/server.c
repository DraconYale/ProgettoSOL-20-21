#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#include <boundedBuffer.h>
#include <txtParser.h>
#include <functions.h>
#include <storage.h>

#define BUFFERSIZE 256
#define UNIX_PATH_MAX 108

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


int main (int argc, char** argv){

	if(argc != 2){
		printf("Usage: ./server '/path/to/config'\n");
		return 0;
	}
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
	if((logFile = fopen("./log.txt", w+)) == NULL){
		perror("fopen");
		return -1;
	}
	
	//starting threads (to be fully implemented) 
	pthred_t* workerThreads;
	if((workerThreads = malloc(config->workerNumber * sizeof(pthread_t))) == NULL){
		perror("malloc");
		return -1;
	}
	int i = 0;
	while(i < config->workerNumber){
		
		if((pthread_create(&(workerThreads[i], NULL, function, (void*) args))) != 0){
			perror("pthread_create workers");
			return -1;
		}
		i++;
	
	}
	return 0;




}
