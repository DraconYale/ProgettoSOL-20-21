#include <boundedBuffer.h>
#include <txtParser.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <socket.h>
#include <signal.h>

#define BUFFERSIZE 256


int main (int argc, char** argv){
	if(argc != 2){
		printf("Usage: ./server '/path/to/config'\n");
		return 0;
	}
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
