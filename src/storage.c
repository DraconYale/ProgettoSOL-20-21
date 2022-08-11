#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <list.h>
#include <hashtable.h>
#include <locker.h>
#include <functions.h>

#define MAXFILELEN 24
#define MAXLEN 256

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
	
	if((newStorage = malloc(sizeof(storage))) == NULL){
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

int storageOpenFile(storage* storage, char* filename, int flags, int client){
	
	if(storage == NULL || filename == NULL){
		errno = EINVAL;
		return -1;
	}
	if(flags & O_CREATE){
		//enters as a writer
		if(writeLock(storage->mux) != 0){
			return -2;
		}
		if(hashSearch(storage->files, filename) != NULL){
			if(writeUnlock(storage->mux) != 0){
				return -2;
			}
			errno = EEXIST;					//EEXIST 17 File già esistente (from "errno -l")
			return -1;
		}
		storedFile* newFile;
		if((newFile = malloc(sizeof(storedFile))) == NULL){
			return -2;
		}
		
		if((newFile->name = malloc(strlen(filename)*sizeof(char)+1)) == NULL){
			return -2;
		}
		strcpy(newFile->name, filename);
		newFile->size = 0;
		newFile->content = NULL;
		newFile->whoOpened = initList();
		int length = snprintf(NULL, 0, "%d", client);
		char* clientStr;
		if(clientStr = malloc(length + 1)){
			return -2
		}
		snprintf(clientStr, length + 1, "%d", client);
		appendList(newFile->whoOpened, clientStr);
		if(flags & O_LOCK){
			newFile->lockerClient = client;
		}
		else{
			newFile->lockerClient = -1;
		}
		newFile->lastAccess = time(NULL);
		hashInsert(storage->files, filename, (void*)newFile);
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		return 0;
	}
	else{
		//enters as a reader
		if(readLock(storage->mux) != 0){
			return -2;
		}
		storedFile* openF;
		if((openF = hashSearch(storage->files, filename)) == NULL){
			if(readUnlock(storage->mux) != 0){
				return -2;
			}
			errno = ENOENT;					//ENOENT 2 File o directory non esistente (from "errno -l")
			return -1;
		}
		//it is needed to modify whoOpened and clientLocker (if O_LOCK flag is set)
		if(writeLock(openF->mux) != 0){
			return -2;
		}
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		if(flags & O_LOCK){
			if(openF->clientLocker == -1 || openF->clientLocker == client){
				openF->clientLocker = client;
			}
			else{
				if(writeUnlock(openF->mux) != 0){
					return -2;
				}
				errno = EACCES 				//EACCES 13 Permesso negato (from "errno -l")
				return -1;				
			}
		}
		int length = snprintf(NULL, 0, "%d", client);
		char* clientStr;
		if(clientStr = malloc(length + 1)){
			return -2
		}
		snprintf(clientStr, length + 1, "%d", client);
		if((containsList(openF->whoOpened, clientStr)) == 0){
			appendList(newFile->whoOpened, clientStr);
		}
		openF->lastAccess = time(NULL);
		if(writeUnlock(openF->mux) != 0){
			return -2;
		}	
	
	
	}
	return 0;
}

long storageReadFile(storage* storage, char* filename, void** sentCont, int client){
	
	long bytesSent;
	if(storage == NULL || filename == NULL){
		errno = EINVAL;
		return 0;
	}
	if(readLock(&(storage->mux)) != 0){
		return -2;
	}
	storedFile* readF;
	if((readF = hashSearch(storage->files)) == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(readLock(readF->mux) != 0){
		return -2;
	}
	if(readUnlock(storage->mux) != 0){
		return -2;
	}
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if(clientStr = malloc(length + 1)){
		return -2
	}
	snprintf(clientStr, length + 1, "%d", client);
	if((containsList(readF->whoOpened, clientStr)) != 0){
		if(readUnlock(readF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	}
	if(readF->clientLocker == -1 || readF->clientLocker == client){
		if(readF->size == 0){
			*sentCont = "";
		}
		else{
			if((*sentCont = malloc(readF->size)) == NULL){
				return -2;
			}
			memcpy(*sentCont, readF->content, readF->size);
			bytesSent = readF->size;
			if(readUnlock(readF->mux) != 0){
				return -2;
			}
			return bytesSent;
		}
	
	}
	else{
		if(readUnlock(readF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	return 0;
	
}

