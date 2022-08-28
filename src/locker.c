#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <locker.h>

//lock system to manage read/write concurrent operations
//pthread mutex and condition are used to modify the locker internal state

struct locker{
	pthread_mutex_t mux;
	pthread_cond_t cond;
	bool writerPresence;
	int readersNumber;
};

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
	
	return newLocker;
}


int readLock(locker* lock){
	int err;
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if((err = pthread_mutex_lock(&(lock->mux))) != 0){
		errno = err;
		return -1;					//fatal error
	}
	while(lock->writerPresence){				//waiting for writer/s to finish
		pthread_cond_wait(&(lock->cond), &(lock->mux));
	}
	lock->readersNumber++;					//a reader joined the reader's group
	if((err = pthread_mutex_unlock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}
	
	return 0;
}

int readUnlock(locker* lock){
	int err;
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if((err = pthread_mutex_lock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}
	lock->readersNumber--;					//a reader left the reader's group
	if(lock->readersNumber == 0){
		pthread_cond_broadcast(&(lock->cond));
	}					
	if((err = pthread_mutex_unlock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}
	
	return 0;

}

int writeLock(locker* lock){
	int err;
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if((err = pthread_mutex_lock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}	
	while(lock->writerPresence){					//writer waits for other writers to finish
		pthread_cond_wait(&(lock->cond), &(lock->mux));
	}					
	lock->writerPresence = true;
	while(lock->readersNumber != 0){			//writer waits for readers to finish
		pthread_cond_wait(&(lock->cond), &(lock->mux));
	}	
				
	if((err = pthread_mutex_unlock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}
	
	return 0;

}

int writeUnlock(locker* lock){
	int err;
	if(lock == NULL){
		errno = EINVAL;
		return -1;
	}
	if((err = pthread_mutex_lock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}				
	lock->writerPresence = false;
	pthread_cond_broadcast(&(lock->cond));			
	if((err = pthread_mutex_unlock(&(lock->mux))) != 0){
		errno = err;
		return -1;
	}
	
	return 0;
}

int freeLock(locker* lock){
	if(lock == NULL){
		return 0;
	}
	pthread_mutex_destroy(&(lock->mux));
	pthread_cond_destroy(&(lock->cond));
	free(lock);
	return 0;
}


