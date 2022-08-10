#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include <locker.h>

//lock system to manage read/write concurrent operations
//pthread mutex and condition are used to modify the locker internal state

struct locker{
	pthread_mutex_t mux;
	pthread_cond_t cond;
	bool writerPresence;
	int readersNumber;
}

locker* lockerInit(){
	int err;
	pthread_mutex_t newMux;
	if((err = pthread_mutex_init(&newMux, NULL)) != 0){
		errno = err;
		return NULL;
	}
	pthread_cond_t newCond;
	if((err = pthread_cond_init(&newCond, NULL)) != 0){
		errno = err;
		pthread_mutex_destroy(&newMux);
		return NULL;
	}
	locker* newLocker = malloc(sizeof(locker));
	if(newLocker == NULL){
		pthread_mutex_destroy(&newMux);
		pthread_cond_destroy(&newCond);
		return NULL;	
	}
	newLocker->mux = newMux;
	newLocker->cond = newCond;
	newLocker->writerPresence = false;
	newLocker->readersNumber = 0;
	
	return newLock;
}


int readLock(locker* lock){
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if(pthread_mutex_lock(&(lock->mux)) != 0){
		return -1;					//fatal error
	}
	while(lock->writerPresence){				//waiting for writer/s to finish
		pthread_cond_wait(&(lock->cond), &(lock->mux));
	}
	lock->readersNumber++;					//a reader joined the reader's group
	if(pthread_mutex_unlock(&(lock->mux)) != 0){
		return -1;
	}
	
	return 0;
}

int readUnlock(locker* lock){
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if(pthread_mutex_lock(&(lock->mux)) != 0){
		return -1;
	}
	lock->readersNumber--;					//a reader left the reader's group
	if(lock->readersNumber == 0){
		pthread_cond_signal(&(lock->cond));
	}					
	if(pthread_mutex_unlock(&(lock->mux)) != 0){
		return -1;
	}
	
	return 0;

}


