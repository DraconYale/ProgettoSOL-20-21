#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include <icl_hash.h>
#include <list.h>
#include <locker.h>
#include <functions.h>
#include <storage.h>


#define MAXFILELEN 24
#define MAXLEN 256

//repPolicy = 0 ==> FIFO
//repPolicy = 1 ==> LRU

void freeFile(storedFile* file){
	if(file == NULL){
		return;
	}
	if(file->name != NULL){
		free(file->name);
	}
	if(file->content != NULL){
		free((file->content));
	}
	if(file->whoOpened != NULL){
		freeList(file->whoOpened, (void*)freeElem);
	}
	if(file->mux != NULL){
		freeLock(file->mux);
	}
	file->size = 0;
	file->lockerClient = -1;
	if(file != NULL){
		free(file);
		file = NULL;
	}	
}

storage* storageInit(int maxFiles, unsigned long maxMB, int repPolicy){

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
	
	if(repPolicy != 0 && repPolicy != 1){
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
	
	newStorage->files = icl_hash_create((int)maxFiles, hash_pjw, string_compare);
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
	elem* victimElem = NULL;
	char* vName = NULL;
	storedFile* victim = NULL;
	storedFile* copyV = NULL;
	elem* tmpElem = NULL;
	switch(storage->repPolicy){
		
		//FIFO policy
		case 0:
			victimElem = getHead(storage->filesFIFOQueue);
			vName = victimElem->info;
			victim = icl_hash_find(storage->files, (void*)vName);
			copyV = malloc(sizeof(storedFile));
			if((copyV->name = malloc((strlen(victim->name))*sizeof(char)+1)) == NULL){
				return NULL;
			}
			memcpy(copyV->name, victim->name, strlen(victim->name)+1);
			copyV->size = victim->size;
			if((copyV->content = malloc(victim->size)) == NULL){
					return NULL;
			}
			memcpy(copyV->content, victim->content, victim->size);
			copyV->whoOpened = NULL;
			copyV->mux = NULL;
			storage->filesNumb--;
			storage->sizeMB = storage->sizeMB - victim->size;
			storage->victimNumb++;
			removeList(storage->filesFIFOQueue, vName, (void*)freeElem);
			icl_hash_delete(storage->files, victim->name, free, (void*)freeFile);
			return copyV;
	
		
		//LRU policy
		case 1:
			victimElem = getHead(storage->filesFIFOQueue);
			vName = victimElem->info;
			victim = icl_hash_find(storage->files, (void*) vName);
			tmpElem = nextList(storage->filesFIFOQueue, victimElem);
			if(tmpElem != NULL){
				
				storedFile* tmpVict = icl_hash_find(storage->files, (void*) tmpElem->info);
				long tmpTime = 0;
				long maxDiffTime = fabs(difftime(victim->lastAccess, tmpVict->lastAccess));
				
				while(tmpElem != NULL){
					tmpTime = difftime(victim->lastAccess, tmpVict->lastAccess);
					if(fabs(tmpTime) > fabs(maxDiffTime)){
						maxDiffTime = fabs(tmpTime);
						if(tmpTime > 0){
							victim = tmpVict;
						}
				
					}
					tmpElem = nextList(storage->filesFIFOQueue, tmpElem);
					if(tmpElem != NULL){
						tmpVict = icl_hash_find(storage->files, (void*) tmpElem->info);
					}
					
				}
			}
			
			copyV = malloc(sizeof(storedFile));
			if((copyV->name = malloc(strlen(victim->name)*sizeof(char)+1)) == NULL){
				return NULL;
			}
			memcpy(copyV->name, victim->name, strlen(victim->name)+1);
			copyV->size = victim->size;
			if((copyV->content = malloc(victim->size)) == NULL){
				return NULL;
			}
			memcpy(copyV->content, victim->content, victim->size);
			copyV->whoOpened = NULL;
			copyV->mux = NULL;
			storage->filesNumb--;
			storage->sizeMB = storage->sizeMB - victim->size;
			storage->victimNumb++;
			removeList(storage->filesFIFOQueue, victim->name, (void*)freeElem);
			icl_hash_delete(storage->files, victim->name, free, (void*)freeFile);
			return copyV;
	}
	return NULL;
}

int updateStorage(storage* storage){
	if(storage->filesNumb > storage->maxFileStored){
		storage->maxFileStored = storage->filesNumb;
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
	if(writeLock(storage->mux) != 0){
		return -2;
	}
	int found = 0;
	char* tmpFilename = malloc(strlen(filename)*sizeof(char)+1);
	tmpFilename[strlen(filename)] = '\0';
	strncpy(tmpFilename, filename, strlen(filename));
	if((icl_hash_find(storage->files, (void*) tmpFilename) != NULL)){
		found = 1;
	}
	if(flags & O_CREATE && found == 1){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		free(tmpFilename);
		errno = EEXIST;
		return -1;
	}
	if(flags & O_CREATE && found == 0){
		//enters as a writer
		storedFile* newFile;
		if((newFile = malloc(sizeof(storedFile))) == NULL){
			return -2;
		}
		
		if((newFile->name = malloc(strlen(filename)*sizeof(char)+1)) == NULL){
			return -2;
		}
		strncpy(newFile->name, filename, strlen(filename)+1);
		newFile->name[strlen(filename)] = '\0';
		newFile->size = 0;
		newFile->content = NULL;
		newFile->whoOpened = initList();
		newFile->mux = lockerInit();
		int length = snprintf(NULL, 0, "%d", client);
		char* clientStr;
		if((clientStr = malloc(length + 1)) == NULL){
			return -2;
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
		icl_hash_insert(storage->files, (void*)tmpFilename, (void*)newFile);
		free(clientStr);
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		return 0;
	}
	else{
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		//enters as a reader
		if(readLock(storage->mux) != 0){
			return -2;
		}
		storedFile* openF;
		if((openF = icl_hash_find(storage->files, (void*)tmpFilename)) == NULL){
			if(readUnlock(storage->mux) != 0){
				return -2;
			}
			free(tmpFilename);
			errno = ENOENT;					//ENOENT 2 File o directory non esistente (from "errno -l")
			return -1;
		}
		//it is needed to modify whoOpened and lockerClient (if O_LOCK flag is set)
		if(writeLock(openF->mux) != 0){
			return -2;
		}
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		if(flags & O_LOCK){
			if(openF->lockerClient == -1 || openF->lockerClient == client){
				openF->lockerClient = client;
			}
			else{
				if(writeUnlock(openF->mux) != 0){
					return -2;
				}
				free(tmpFilename);
				errno = EACCES; 				//EACCES 13 Permesso negato (from "errno -l")
				return -1;				
			}
			
		}
		int length = snprintf(NULL, 0, "%d", client);
		char* clientStr;
		if((clientStr = malloc(length + 1)) == NULL){
			return -2;
		}
		snprintf(clientStr, length + 1, "%d", client);
		if((containsList(openF->whoOpened, clientStr)) == 0){
			appendList(openF->whoOpened, clientStr);
		}
		free(clientStr);
		openF->lastAccess = time(NULL);
		if(writeUnlock(openF->mux) != 0){
			return -2;
		}	
	
	
	}
	free(tmpFilename);
	return 0;
}

int storageReadFile(storage* storage, char* filename, void** sentCont, unsigned long* sentSize, int client){
	
	if(storage == NULL || filename == NULL){
		errno = EINVAL;
		return 0;
	}
	if(readLock(storage->mux) != 0){
		return -2;
	}
	storedFile* readF;
	if((readF = icl_hash_find(storage->files, (void*)filename)) == NULL){
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
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if((containsList(readF->whoOpened, clientStr)) == 0){
		if(readUnlock(readF->mux) != 0){
			return -2;
		}
		free(clientStr);
		errno = EACCES;
		return -1;
	}
	free(clientStr);
	if(readF->lockerClient == -1 || readF->lockerClient == client){
		if(readF->size == 0){
			*sentCont = NULL;
			*sentSize = readF->size;
			return 0;
		}
		else{
			if((*sentCont = malloc(readF->size)) == NULL){
				return -2;
			}
			memcpy(*sentCont, readF->content, readF->size);
			*sentSize = readF->size;
			readF->lastAccess = time(NULL);
			if(readUnlock(readF->mux) != 0){
				return -2;
			}
			return 0;
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
	if(storage == NULL){
		errno = EINVAL;
		return -1;
	}
	int maxReached = 0;
	int counter = 0;
	if(readLock(storage->mux) != 0){
		return -2;
	}
	if(N <= 0){
		if(storage->filesNumb > 0){
			N = storage->filesNumb;
		}
		else{
			if(readUnlock(storage->mux) != 0){
				return -2;
			}
			errno = ENOENT;
			return -1;
		}
		
	}
	elem* listElem = getHead(storage->filesFIFOQueue);
	if(listElem == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		return -1;
	}
	list* tmpList = initList();
	while(counter < N && !maxReached){
		
		storedFile* kFile = icl_hash_find(storage->files, listElem->info);
		storedFile* file = NULL;
		if(readLock(kFile->mux) != 0){
			return -2;
		}
		if(kFile->lockerClient == client || kFile->lockerClient == -1){
			//a copy of the read file is used
			if((file = malloc(sizeof(storedFile))) == NULL){
				return -2;
			}
			memset(file, 0, sizeof(storedFile));
			if((file->name = malloc(strlen(kFile->name)*sizeof(char)+1)) == NULL){
				return -2;
			}
			strncpy(file->name, kFile->name, strlen(kFile->name)+1);
			file->size = kFile->size;
			if((file->content = malloc(kFile->size)) == NULL){
				return -2;
			}
			memcpy(file->content, kFile->content, kFile->size);			
			
			if(appendListCont(tmpList, file->name, strlen(file->name), file->content, file->size) == NULL){
				if(readUnlock(kFile->mux) != 0){
					return -2;
				}
				if(readUnlock(storage->mux) != 0){
					return -2;
				}
				freeFile(file);
				return -1;
			}
			freeFile(file);
			kFile->lastAccess = time(NULL);
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
	*sentFilesList = tmpList;
	return 0;
}

//storageWriteFile fails if the file is not "new" or locked by client (file needs to be opened wit O_CREATE and O_LOCK)
int storageWriteFile(storage* storage, char* name, void* content, unsigned long contentSize, list** victims, int client){
	
	if(storage == NULL || name == NULL || content == NULL || contentSize <= 0 || victims == NULL){
		errno = EINVAL;
		return -1;
	}
	if(writeLock(storage->mux) != 0){
		return -2;
	}
	storedFile* writeF;
	if((writeF = icl_hash_find(storage->files, (void*)name)) == NULL){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(writeF->mux) != 0){
		return -2;
	}
	
	//check if client opened this file
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(writeF->whoOpened, clientStr)){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeLock(writeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	
	//check if file is created with O_CREATE
	if(writeF->size != 0){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeUnlock(writeF->mux) != 0){
			return -2;
		}
		errno = EEXIST;
		return -1;
	}
	
	//check if client has file lock (O_LOCK flag)
	if(writeF->lockerClient != client){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeUnlock(writeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;	
	}
	if((storage->filesNumb + 1) > storage->maxFiles || (storage->sizeMB + contentSize) > storage->maxMB){
		list* tmpVictims = initList();
	
	//check if there is enough space
		while((storage->filesNumb + 1) > storage->maxFiles || (storage->sizeMB + contentSize) > storage->maxMB){
				storedFile* victim; 
				if((victim = getVictim(storage)) == NULL){
					return -2;
				}
				appendListCont(tmpVictims, victim->name, strlen(victim->name), victim->content, victim->size);
				freeFile(victim);
		}
		*victims = tmpVictims;
	}
	char* tmpName = malloc(strlen(writeF->name)*sizeof(char)+1);
	strncpy(tmpName, writeF->name, strlen(writeF->name)+1);
	char* data = malloc(contentSize+1);
	strncpy(data, content, contentSize+1);
	writeF->content = data;
	writeF->size = contentSize;
	writeF->lastAccess = time(NULL);
	storage->filesNumb++;
	storage->sizeMB = storage->sizeMB + contentSize;
	appendList(storage->filesFIFOQueue, tmpName);
	updateStorage(storage);
	free(tmpName);
	if(writeUnlock(writeF->mux) != 0){
		return -2;
	}
	if(writeUnlock(storage->mux) != 0){
		return -2;
	}
	free(clientStr);
	
	return 0;
	

}

int storageAppendFile(storage* storage, char* name, void* content, unsigned long contentSize, list** victims, int client){
	
	if(storage == NULL || name == NULL || content == NULL || contentSize <= 0 || victims == NULL){
		errno = EINVAL;
		return -1;
	}
	if(writeLock(storage->mux) != 0){
		return -2;
	}
	storedFile* writeF;
	if((writeF = icl_hash_find(storage->files, (void*)name)) == NULL){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(writeF->mux) != 0){
		return -2;
	}
	
	//check if client opened this file
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(writeF->whoOpened, clientStr)){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeUnlock(writeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	
	if(writeF->lockerClient == -1 || writeF->lockerClient == client){	
		//check if there is enough space
		while(storage->filesNumb + 1 > storage->maxFiles || storage->sizeMB + contentSize > storage->maxMB){
				storedFile* victim; 
				if((victim = getVictim(storage)) == NULL){
					return -2;
				}
				appendListCont(*victims, victim->name, strlen(victim->name), victim->content, victim->size);
		}
		
		if((writeF->content = realloc(writeF->content, writeF->size + contentSize)) == NULL){
			return -2;
		}
		memcpy(writeF->content + writeF->size, content, contentSize);
		writeF->size = writeF->size + contentSize;
		writeF->lastAccess = time(NULL);	
		storage->sizeMB = storage->sizeMB + contentSize;
		updateStorage(storage);
		if(writeUnlock(writeF->mux) != 0){
			return -2;
		}
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		
		return 0;
	}
	else{
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeLock(writeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	}
}

int storageLockFile(storage* storage, char* name, int client){
	
	if(storage == NULL || name == NULL){
		errno = EINVAL;
		return -1;
	}
	if(readLock(storage->mux) != 0){
		return -2;
	}
	storedFile* lockF;
	if((lockF = icl_hash_find(storage->files, (void*)name)) == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(lockF->mux) != 0){
		return -2;
	}
	if(readUnlock(storage->mux) != 0){
		return -2;
	}
	
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(lockF->whoOpened, clientStr)){
		if(writeUnlock(lockF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	free(clientStr);
	if(lockF->lockerClient == -1 || lockF->lockerClient == client){
		lockF->lockerClient = client;
		lockF->lastAccess = time(NULL);
		if(writeUnlock(lockF->mux) != 0){
			return -2;
		}
		return 0;
	}
	else{
		if(writeUnlock(lockF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	}
}

int storageUnlockFile(storage* storage, char* name, int client){
	
	if(storage == NULL || name == NULL){
		errno = EINVAL;
		return -1;
	}
	if(readLock(storage->mux) != 0){
		return -2;
	}
	storedFile* unlockF;
	if((unlockF = icl_hash_find(storage->files, (void*)name)) == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(unlockF->mux) != 0){
		return -2;
	}
	if(readUnlock(storage->mux) != 0){
		return -2;
	}
	
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(unlockF->whoOpened, clientStr)){
		if(writeUnlock(unlockF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	free(clientStr);
	
	if(unlockF->lockerClient == -1 || unlockF->lockerClient == client){
		unlockF->lockerClient = -1;
		if(writeUnlock(unlockF->mux) != 0){
			return -2;
		}
		return 0;
	}
	else{
		if(writeUnlock(unlockF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	}
}

int storageCloseFile(storage* storage, char* filename, int client){
	
	if(storage == NULL || filename == NULL){
		errno = EINVAL;
		return -1;	
	}
	
	if(readLock(storage->mux) != 0){
		return -2;
	}
	storedFile* closeF;
	if((closeF = icl_hash_find(storage->files, (void*)filename)) == NULL){
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(readLock(closeF->mux) != 0){
		return -2;
	}
	
	
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	
	if(!containsList(closeF->whoOpened, clientStr)){
		if(readUnlock(closeF->mux) != 0){
			return -2;
		}
		if(readUnlock(storage->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}else{
		if(readUnlock(closeF->mux) != 0){
			return -2;
		}
		if(writeLock(closeF->mux) != 0){
			return -2;
		}
		removeList(closeF->whoOpened, clientStr, (void*)freeElem);
		if(writeUnlock(closeF->mux) != 0){
			return -2;
		}
	}
	free(clientStr);
	if(readUnlock(storage->mux) != 0){
		return -2;
	}
	return 0;
	
}

int storageRemoveFile(storage* storage, char* name, int client){
	
	if(storage == NULL || name == NULL){
		errno = EINVAL;
		return -1;
	}
	
	if(writeLock(storage->mux) != 0){
		return -2;
	}
	storedFile* removeF;
	if((removeF = icl_hash_find(storage->files, (void*)name)) == NULL){
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		errno = ENOENT;					
		return -1;
	}
	if(writeLock(removeF->mux) != 0){
		return -2;
	}
	
	int length = snprintf(NULL, 0, "%d", client);
	char* clientStr;
	if((clientStr = malloc(length + 1)) == NULL){
		return -2;
	}
	snprintf(clientStr, length + 1, "%d", client);
	if(!containsList(removeF->whoOpened, clientStr)){
		if(writeUnlock(removeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
	
	}
	free(clientStr);
	
	//file needs to be locked
	if(removeF->lockerClient == client){
		storage->filesNumb--;
		storage->sizeMB = storage->sizeMB - removeF->size;
		removeList(storage->filesFIFOQueue, removeF->name, (void*)freeElem);
		icl_hash_delete(storage->files, removeF->name, free, (void*)freeFile);
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		return 0;
		
	}
	else{
	
		if(writeUnlock(storage->mux) != 0){
			return -2;
		}
		if(writeUnlock(removeF->mux) != 0){
			return -2;
		}
		errno = EACCES;
		return -1;
		
	}
}

int freeStorage(storage* storage){
	if(storage == NULL){
		return 0;
	}
	if(writeLock(storage->mux) != 0){
		return -2;
	}
	if(storage->files != NULL){
		icl_hash_destroy(storage->files, free, (void (*)(void *))freeFile);
	}
	if(storage->mux != NULL){
		freeLock(storage->mux);
	}
	if(storage->filesFIFOQueue != NULL){
		freeList(storage->filesFIFOQueue, (void*)freeElem);
	}
	free(storage);
	return 0;
}

