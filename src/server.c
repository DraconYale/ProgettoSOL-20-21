#include <boundedBuffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <socket.h>
#include <signal.h>


int main (int argc, char** argv){
	if(argc != 2){
		printf("Usage: ./server '/path/to/config'\n");
		return 0;
	}
	return 0;




}
