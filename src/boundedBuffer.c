/*
Bounded buffer used to create the job queue. It's implemented as a circular queue (this may change)
*/

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

int jobNumber = 0;

struct boundedBuffer{
	size_t size;
	char** head;
	char** commands;						//circular queue
	char** tail;
	pthread_mutex_t mutex;
	pthread_cond_t full;
	pthread_cond t empty;
}


boundedBuffer* initBuffer(size_t size){
	if(size <= 0){
		errno = EINVAL;						//EINVAL 22 Argomento non valido (from 'errno -l')
		return NULL;
	}
	int err;
	if((char** commands = calloc(size, sizeof(char*))) == NULL){
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
	buf->head = commands;
	buf->tail = commands;
	buf->mutex = mutex;
	buf->full = full;
	buf->empty = empty;
	return buf;
}


int enqueueBuffer(boundedBuffer* buf, char* op){
	if(buf == NULL || op == NULL){
		errno = EINVAL;
		return -1;
	}
	int err;
	
	//critical section: need mutex to enqueue new job
	if((err = (pthread_mutex_lock(&(buf->mutex)))) != 0){
		perror("Couldn't lock");
		return -1;
	}
	while(buf->size == jobNumber){
		pthread_cond_wait(&(buf->full), &(buf->mutex));
	}
	*(buf->tail) = op;
	jobNumber++;
	*(buf->tail)+1;
	if(jobNumber == (buf->size)){
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
	int err
	
	//critical section
	if((err = (pthread_mutex_lock(&(buf->mutex)))) != 0){
		perror("Couldn't lock");
		return -1;
	}
	while(jobNumber == 0){
		pthread_cond_wait(&(buf->empty, &(buf->mutex));
	}
	*retjob = *(buf->head);
	*(buf->head)+1;
	jobNumber--;
	if(jobNumber == 0){
		pthread_cond_broadcast(&(buf->full));
	}
	if((err = (pthread_mutex_unlock(&(buf->mutex)))) != 0){
		perror("Couldn't unlock");
		return -1;
	}
}


