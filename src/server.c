#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <socket.h>
#include <signal.h>

#include <boundedBuffer.h>
#include <txtParser.h>
#include <functions.h>
#include <storage.h>


#define BUFFERSIZE 256

volatile sig_atomic_t term = 0; 		//term = 1 when SIGINT or SIGQUIT
volatile sig_atomic_t blockNewClients = 0; 	//blockNewClients = 1 when SIGHUP



void* signalHandling(void* set){
	sigset_t* copySet = (sigset_t*) set;
	int sig;
	while(1){
		if((sigwait(copyset, &sig)) == 0){
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
	pthread_t sigHandler;
	struct sigaction s;
	sigset_t set;
	
	//signal handling
	memset(&s, 0, sizeof(s));
	s->sa_handler = SIG_IGN;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGQUIT);
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
	
	//TODO
	
	
	
	
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
	
	return 0;




}
