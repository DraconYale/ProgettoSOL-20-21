#include <txtParser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//define config file "format"
#define WORKERS "Worker Number = "
#define STORAGEB "Storage Size = "
#define STORAGEF "Storage File Number = "
#define SOCKET "Path To Socket = "

#define MAX_SOCKET_LENGHT 256
#define BUFFERSIZE 1024

struct txtFile{
	long workerNumber;			//worker threads number
	long storageSize;			//storage size in bytes
	long storageFileNumber;			//max file number in storage
	char* pathToSocket;
}


txtFile* txtInit(){
	txtFile* conf = malloc(sizeof(txtFile));
	if(conf == NULL){
		errno = ENOMEM			//ENOMEM 12 Impossibile allocare memoria (from: 'errno -l')
		return NULL;
	}
	conf->workerNumber = 0;
	conf->storageSize = 0;
	conf->storageFileNumber = 0;
	conf->pathToSocket = calloc(MAX_SOCKET_LENGHT, sizeof(char));
	return conf;
}

int applyConfig(txtFile* conf, const char* pathToConfig){
	if(conf == NULL || pathToConfig == NULL){
		errno = EINVAL;
		return -1;
	}
	FILE* config;
	if((config = fopen(pathToConfig, "r")) == NULL){						//fopen sets errno when fails
		return -1;
	}
	size_t i;
	char* buf = calloc(BUFFERSIZE, sizeof(char));
	long value;
	char* tmp_string;
	//fgets reads BUFFERSIZE bytes from conf and saves them in buf and stops after '\n' or newline							
	while(!feof(config)){
		fgets(buf, BUFFERSIZE, config);			
		//parser needs to search for every defined config in configuration file
		if(strncmp(buf, WORKERS, strlen(WORKERS)) == 0){
			value = strtol(buf + strlen(WORKERS), NULL, 10); 				//strtol of value, base 10
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				cleanconf(config);
				errno = EINVAL;								//value not valid, sets errno = EINVAL
				return -1;
			}
			else{
				conf->workerNumber = value;
				break;
			}
		}
		if(strncmp(buf, STORAGEB, strlen(STORAGEB)) == 0){
			value = strtol(buf + strlen(STORAGEB), NULL, 10);
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				cleanconf(config);
				errno = EINVAL;								//value not valid, sets errno = EINVAL
				return -1;
			}
			else{
				conf->storageSize = value;
				break;
			}
		}
		if(strncmp(buf, STORAGEF, strlen(STORAGEF)) == 0){
			value = strtol(buf + strlen(STORAGEF), NULL, 10); 				//strtol of value, base 10
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				cleanconf(config);
				errno = EINVAL;								//value not valid, sets errno = EINVAL
				return -1;
			}
			else{
				conf->storageFileNumber = value;
				break;
			}
		}
		if(strncmp(buf, SOCKET, strlen(SOCKET)) == 0){
			strncpy(conf->pathToSocket, buf+strlen(SOCKET), MAX_SOCKET_LENGHT); 		//strtol of value, base 10
			break;
		}
	}
	if(fclose(config) != 0){									//fclose sets errno
		return -1;
	}
	return 0;
	

}

int cleanconf(txtFile* conf){
	int err;
	free(conf->socket);
	if((err = (fclose(conf))) != 0){			
		return -1;
	}
	return 0;

}
