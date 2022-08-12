#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
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


storedFile* getVictim(storage* storage){
	switch(storage->repPolicy){
		
		//FIFO policy
		case 0:
			elem* victimElem = popHead(storage->filesFIFOQueue);
			char* victimName = victimElem->info;
			storedFile* victim = hashSearch(storage->files, victimName);
			if(writeLock(victim->mux) != 0){
				return NULL;
			}
			storedFile* copyV = malloc(sizeof(storedFile));
			if((copyV->name = malloc(strlen(victim->name)*sizeof(char)+1)) == NULL){
				return NULL;
			}
			memcpy(copyV->name, victim->name, strlen(victim->name)+1);
			copyV->size = victim->size;
			if((copyV->content = malloc(victim->size)) == NULL){
				return NULL;
			}
			memcpy(copyV->content, victim->content, victim->size);
			storage->filesNumb--;
			storage->sizeMB = storage->sizeMB - victim->size;
			storage->victimNumb++;
			free(victim->name);
			free(victim->content);
			freeList(victim->whoOpened);
			freeLock(victim->mux);
			hashRemove(storage->files, victim);
			return copyV;		
		
		//LRU policy
		case 1:
			elem* victimElem = getHead(storage->filesFIFOQueue);
			char* victimName = victimElem->info;
			storedFile* victim = hashSearch(storage->files, victimName);
			elem* tmpElem = nextList(storage->filesFIFOQueue, victimElem);
			if(tmpElem != NULL){
				char tmpName = tmpElem->info;
				storedFile tmpVict = hashSearch(storage->files, tmpName);
				double tmpTime = 0;
				double maxDiffTime = fabs(difftime(victim->lastAccess, tmpVict->lastAccess));
				while(tmpElem != NULL){
					tmpTime = difftime(victim->lastAccess, tmpVict->lastAccess);
					if(fabs(tmpTime) > fabs(maxDiffTime)){
						maxDiffTime = fabs(tmpTime);
						if(tmpTime > 0){
							victim = tmpVict;
						}
				
					}
					tmpElem = nextList(storage->filesFIFOQueue, tmpElem);
					tmpName = tmpElem->info;
					tmpVict = hashSearch(storage-files, tmpName);
				}
			}
			if(writeLock(victim->mux) != 0){
				return NULL;
			}
			storedFile* copyV = malloc(sizeof(storedFile));
			if((copyV->name = malloc(strlen(victim->name)*sizeof(char)+1)) == NULL){
				return NULL;
			}
			memcpy(copyV->name, victim->name, strlen(victim->name)+1);
			copyV->size = victim->size;
			if((copyV->content = malloc(victim->size)) == NULL){
				return NULL;
			}
			memcpy(copyV->content, victim->content, victim->size);
			storage->filesNumb--;
			storage->sizeMB = storage->sizeMB - victim->size;
			storage->victimNumb++;
			free(victim->name);
			free(victim->content);
			freeList(victim->whoOpened);
			freeLock(victim->mux);
			hashRemove(storage->files, victim);
			return copyV;
	}
}

int updateStorage(storage* storage){
	if(storage->filesNumber > storage->maxFileStored){
		storage->maxFileStored = storage->filesNumber;
	}
	if(storage->sizeMB > storage-> maxMBStored){
		storage->maxMBStored = storage->sizeMB;
	}
	return 0;
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
		if((clientStr = malloc(length + 1)) == NULL){
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
		if((clientStr = malloc(length + 1)) == NULL){
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
	if((clientStr = malloc(length + 1)) == NULL){
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

int storageReadNFiles(storage* storage, int N, list** sentFilesList, int client){
	if(storage == NULL || sentContList == NULL){
		errno = EINVAL;
		return -1;
	}
	int maxReached = 0
	int counter = 0;
	if(readLock(&(storage->mux)) != 0){
		return -2;
	}
	if(N <= 0){
		N = storage->filesNumb;
	}
	elem* listElem = storage->filesFIFOQueue;
	if(listElem == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		return -1;
	}
	while(counter < N && !maxReached){
		storedFile* kFile = hashSearch(storage->files, listElem->info);
		storedFile* file = NULL;
		if(readLock(kFile->mux) != 0){
			return -2;
		}
		if(kFile->clientLocker == client || kFile->clientLocker == -1){
			//a copy of the read file is used
			if((file = malloc(sizeof(storedFile)) == NULL){
				return -2;
			}
			memset(file, 0, sizeof(storedFile));
			if((file->name = malloc(strlen(kFile->name)*sizeof(char)+1)) == NULL){
				return -2;
			}
			memcpy(file->name, kFile->name, strlen(kFile->name)+1);
			file->size = kFile->size;
			if((file->content = malloc(kFile->size)) == NULL){
				return -2;
			}
			memcpy(file->content, kFile->content, kFile->size);
			if(appendList(*sentFilesList, file) == NULL){
				if(readUnlock(kFile->mux) != 0){
					return -2;
				}
				if(readUnlock(storage->mux) != 0){
					return -2;
				}
				//TODO free file
				return -1;
			}
			if(readUnlock(kFile->mux) != 0){
				return -2;
			}
			counter++;
		}
		else{
			if(readUnlock(kFile->mux) != 0){
				return -2;
			}
		}
		listElem = nextList(storage->filesFIFOQueue, listElem);
		if(listElem == NULL){
			maxReached = 1;
		}
	}
	if(readUnlock(storage->mux) != 0){
		return -2;
	}
	return counter;
}

//storageWriteFile fails if the file is not "new" or locked by client (file needs to be opened wit O_CREATE and O_LOCK)
int storageWriteFile(storage* storage, char* name, void* content, long contentSize, list** victims, int client){
	
	if(storage == NULL || name == NULL || content == NULL || contentSize <= 0 || victims == NULL){
		errno = EINVAL;
		return -1;
	}
	if(writeLock(&(storage->mux)) != 0){
		return -2;
	}
	storedFile* writeF;
	if((writeF = hashSearch(storage->files, name)) == NULL){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(&(writeF->mux)) != 0){
		return -2;
	}
	
	//check if client opened this file
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(writF->whoOpened, clientStr)){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	
	//check if file is created with O_CREATE
	if(writeF->size != 0){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		errno = EEXIST;
		return -1;
	}
	
	//check if client has file lock (O_LOCK flag)
	if(writeF->clientLocker != client){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;	
	}
	
		
	//check if there is enough space
	while(storage->fileNumber + 1 > storage->maxFiles || storage->sizeMB + contentSize > storage->maxMB){
			storedFile* victim; 
			if((victim = getVictim(storage)) == NULL){
				return -2
			}
			appendList(*victims, victim);
	}
	writeF->content = content;
	writeF->size = contentSize;
	int strLength = strlen(name);
	writeF->name = calloc(strLength, sizeof(char));
	strncpy(writeF->name, name, strLength);
		
	storage->fileNumber++;
	storage->sizeMB = storage->sizeMB + contentSize;
	updateStorage(storage);
	if(writeLock(&(writeF->mux)) != 0){
		return -2;
	}
	if(writeUnlock(&(storage->mux)) != 0){
		return -2;
	}
	
	return 0;
	

}

int storageAppendFile(storage* storage, char* name, void* content, long contentSize, list** victims, int client){
	
	if(storage == NULL || name == NULL || content == NULL || contentSize <= 0 || victims == NULL){
		errno = EINVAL;
		return -1;
	}
	if(writeLock(&(storage->mux)) != 0){
		return -2;
	}
	storedFile* writeF;
	if((writeF = hashSearch(storage->files, name)) == NULL){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(&(writeF->mux)) != 0){
		return -2;
	}
	
	//check if client opened this file
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(writF->whoOpened, clientStr)){
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	
	if(writeF->clientLocker == -1 || writeF->clientLocker == client){	
		//check if there is enough space
		while(storage->fileNumber + 1 > storage->maxFiles || storage->sizeMB + contentSize > storage->maxMB){
				storedFile* victim; 
				if((victim = getVictim(storage)) == NULL){
					return -2
				}
				appendList(*victims, victim);
		}
		
		if((writeF->content = realloc(writeF->content, writeF->size + contentSize)) == NULL){
			return -2;
		}
		memcpy(writeF->content + writeF->size, content, contentSize);
		writeF->size = writeF->size + contentSize;
			
		storage->sizeMB = storage->sizeMB + contentSize;
		updateStorage(storage);
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		
		return 0;
	}
	else{
		if(writeUnlock(&(storage->mux)) != 0){
			return -2;
		}
		if(writeLock(&(writeF->mux)) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	}
}


