#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include <locker.h>

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

