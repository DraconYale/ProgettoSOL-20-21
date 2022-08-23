/*
Bounded buffer used to create the job queue. It's implemented as a circular queue (this may change)
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <boundedBuffer.h>

struct boundedBuffer{
	size_t size;
	int jobNumber;
	int head;							
	int tail;
	char** commands;						//circular queue
	pthread_mutex_t mutex;
	pthread_cond_t full;
	pthread_cond_t empty;
};


boundedBuffer* initBuffer(int size){
	if(size <= 0){
		errno = EINVAL;						//EINVAL 22 Argomento non valido (from: 'errno -l')
		return NULL;
	}
	char** commands;
	if((commands = calloc(size, sizeof(char*))) == NULL){
		perror("calloc");
		exit(EXIT_FAILURE);
	
	}
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t full = PTHREAD_COND_INITIALIZER;
	pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
	boundedBuffer* buf = NULL;
	if(((buf = malloc(sizeof(boundedBuffer)))) == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	
	}
	buf->size = size;
	buf->jobNumber = 0;
	buf->head = 0;
	buf->tail = 0;
	buf->commands = commands;
	buf->mutex = mutex;
	buf->full = full;
	buf->empty = empty;
	return buf;
}


int enqueueBuffer(boundedBuffer* buf, char* job){
	if(buf == NULL || job == NULL){
		errno = EINVAL;
		return -1;
	}
	int err;
	
	//critical section: need mutex to enqueue new job
	if((err = (pthread_mutex_lock(&(buf->mutex)))) != 0){
		perror("Couldn't lock");
		return -1;
	}
	while(buf->size == buf->jobNumber){
		pthread_cond_wait(&(buf->full), &(buf->mutex));
	}
	(buf->commands[buf->head]) = job;
	buf->head = (buf->head + 1) % buf->size;
	buf->jobNumber = buf->jobNumber + 1;
	if(buf->jobNumber == (buf->size)){
		pthread_cond_broadcast(&(buf->empty));
	}
	if((err = (pthread_mutex_unlock(&(buf->mutex)))) != 0){
		perror("Couldn't unlock");
		return -1;
	}
	return 0;
}	

int dequeueBuffer(boundedBuffer* buf, char** retjob){
	if(buf == NULL || *retjob == NULL){
		errno = EINVAL;
		return -1;
	}
	int err;
	
	//critical section
	if((err = (pthread_mutex_lock(&(buf->mutex)))) != 0){
		perror("Couldn't lock");
		return -1;
	}
	while(buf->jobNumber == 0){
		pthread_cond_wait(&(buf->empty), &(buf->mutex));
	}
	*retjob = (buf->commands[buf->tail]);
	buf->tail = (buf->tail + 1) % buf->size;
	buf->jobNumber = buf->jobNumber - 1;
	if(buf->jobNumber == 0){
		pthread_cond_broadcast(&(buf->full));
	}
	if((err = (pthread_mutex_unlock(&(buf->mutex)))) != 0){
		perror("Couldn't unlock");
		return -1;
	}
	return 0;
}

int cleanBuffer(boundedBuffer* buf){
	free(buf->commands);
	pthread_mutex_destroy(&(buf->mutex));
	pthread_cond_destroy(&(buf->full));
	pthread_cond_destroy(&(buf->empty));
	free(buf);
	return 0;
}


