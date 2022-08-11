#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <list.h>
#include <hashtable.h>
#include <locker.h>

#define MAXFILELEN 24

//repPolicy = 0 ==> FIFO
//repPolicy = 1 ==> LRU

struct storedFile{
	char* name;
	long size;
	void* content;
	
	int lockerClient;			//client fd that locked the file
	locker* mux;				//mutex used for read/write ops
	
	list* whoOpened;			//list of clients who opened the file
	
	time_t lastAccess;			//used for LRU policy
}

struct storage{

	hashTable* files;			//where files are stored
	int repPolicy;				//replacement policy used
	locker* mux;				//mutex used for read/write ops
	list* filesFIFOQueue;			//list of files used for FIFO replacement
	
	int maxFiles;				//file number upper bound
	long maxMB;				//file size (MB) upper bound
	int filesNumb;				//file number currently stored
	long sizeMB;				//file size (MB) currently stored
	
	int maxFileStored;			//max number of stored files reached
	long maxMBStored;			//max size (MB) of storage reached
	int victimNumb;				//number of victims

}

storage* storageInit(int maxFiles, long maxMB, int repPolicy){

	if(maxFiles <= 0 || maxMB <= 0){
		errno = EINVAL;
		return NULL;
	}
	storage* newStorage;
	
	if(newStorage = malloc(sizeof(storage)) == NULL){
		return NULL;
	}
	newStorage->maxFiles = maxFiles;
	newStorage->maxMB = maxMB;
	
	if(repPolicy != 0 || repPolicy != 1){
		newStorage->repPolicy = 0;		//FIFO policy is used if repPolicy is not valid
	}
	else{
		newStorage->repPolicy = repPolicy;
	}
	
	newStorage->filesNumb = 0;
	newStorage->sizeMB = 0;
	newStorage->maxFileStored = 0;
	newStorage->maxMBStored = 0;
	newStorage->victimNumb = 0;
	
	newStorage->files = hashTableInit(maxFiles);
	if(newStorage->files == NULL){
		return NULL;
	}
	
	newStorage->mux = lockerInit();
	if(newStorage->mux == NULL){
		return NULL;
	}
	
	newStorage->filesFIFOQueue = initList();
	if(newStorage->filesFIFOQueue == NULL){
		return NULL;	
	}
	return newStorage;
}




