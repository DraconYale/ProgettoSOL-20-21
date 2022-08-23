#ifndef STORAGE_H_DEFINED
#define STORAGE_H_DEFINED

typedef struct storedFile{

	char* name;
	unsigned long size;
	void* content;
	
	int lockerClient;			//client fd that locked the file
	locker* mux;				//mutex used for read/write ops
	
	list* whoOpened;			//list of clients who opened the file
	
	time_t lastAccess;			//used for LRU policy
	
}storedFile;

typedef struct storage{

	hashTable* files;			//where files are stored
	int repPolicy;				//replacement policy used
	locker* mux;				//mutex used for read/write ops
	list* filesFIFOQueue;			//list of files used for FIFO replacement
	
	int maxFiles;				//file number upper bound
	unsigned long maxMB;				//file size (MB) upper bound
	int filesNumb;				//file number currently stored
	unsigned long sizeMB;				//file size (MB) currently stored
	
	int maxFileStored;			//max number of stored files reached
	unsigned long maxMBStored;			//max size (MB) of storage reached
	int victimNumb;				//number of victims

}storage;

storage* storageInit(int maxFiles, unsigned long maxMB, int repPolicy);

storedFile* getVictim(storage* storage);

int updateStorage(storage* storage);

int storageOpenFile(storage* storage, char* filename, int flags, int client);

int storageReadFile(storage* storage, char* filename, void** sentCont, unsigned long* sentSize, int client);

int storageReadNFiles(storage* storage, int N, list** sentFilesList, int client);

int storageWriteFile(storage* storage, char* name, void* content, unsigned long contentSize, list** victims, int client);

int storageAppendFile(storage* storage, char* name, void* content, unsigned long contentSize, list** victims, int client);

int storageLockFile(storage* storage, char* name, int client);

int storageUnlockFile(storage* storage, char* name, int client);

int storageCloseFile(storage* storage, char* filename, int client);

int storageRemoveFile(storage* storage, char* name, unsigned long* bytesR, int client);

int freeStorage(storage* storage);

#endif

