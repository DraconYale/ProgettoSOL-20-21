#ifndef TXTPARSER_H_DEFINED
#define TXTPARSER_H_DEFINED

typedef struct txtFile{
	long workerNumber;			//worker threads number
	long storageSize;			//storage size in bytes
	long storageFileNumber;			//max file number in storage
	char* pathToSocket;			//path to socket
	long repPolicy;				//0 ==> FIFO 1 ==> LRU
	char* logPath;				//path to log file
}txtFile;

txtFile* txtInit();

int cleanConf(txtFile* conf);

int applyConfig(txtFile* conf, const char* pathToConfig);

#endif
