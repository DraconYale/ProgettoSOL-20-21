#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <txtParser.h>

//define config file "format"
#define WORKERS "Worker Number = "
#define STORAGEB "Storage Size = "
#define STORAGEF "Storage File Number = "
#define SOCKET "Path To Socket = "
#define REPOLICY "Replacement policy = "
#define LOGFILE "Path to log file = "

//other macros
#define MAX_SOCKET_LENGHT 256
#define BUFFERSIZE 1024

txtFile* txtInit(){
	txtFile* conf = malloc(sizeof(txtFile));
	if(conf == NULL){
		errno = ENOMEM;			//ENOMEM 12 Impossibile allocare memoria (from: 'errno -l')
		return NULL;
	}
	conf->workerNumber = 0;
	conf->storageSize = 0;
	conf->storageFileNumber = 0;
	conf->pathToSocket = calloc(MAX_SOCKET_LENGHT, sizeof(char));
	conf->repPolicy = 0;
	conf->logPath = calloc(BUFFERSIZE, sizeof(char));
	return conf;
}

int cleanConf(txtFile* conf){
	free(conf->pathToSocket);
	free(conf->logPath);
	free(conf);
	return 0;

}

int applyConfig(txtFile* conf, const char* pathToConfig){
	if(conf == NULL || pathToConfig == NULL){
		errno = EINVAL;
		return -1;
	}
	FILE* config;
	if((config = fopen(pathToConfig, "r")) == NULL){				//fopen sets errno when fails
		return -1;
	}
	char buf[BUFFERSIZE];
	long value;
	int i = 0;
	//fgets reads BUFFERSIZE bytes from conf and saves them in buf and stops after '\n' or newline							
	while(!feof(config)){
		memset(buf, 0, BUFFERSIZE);
		fgets(buf, BUFFERSIZE, config);			
		//parser needs to search for every defined config in configuration file
		if(strncmp(buf, WORKERS, strlen(WORKERS)) == 0){
			value = strtol(buf + strlen(WORKERS), NULL, 10); 		//strtol of value, base 10
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				if(fclose(config) != 0){				//fclose sets errno
					return -1;
				}
				errno = EINVAL;						//invalid value, sets errno = EINVAL
				return -1;
			}
			else{
				conf->workerNumber = value;
				i++;
				continue;
			}
		}
		if(strncmp(buf, STORAGEB, strlen(STORAGEB)) == 0){
			value = strtol(buf + strlen(STORAGEB), NULL, 10);
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				if(fclose(config) != 0){				//fclose sets errno
					return -1;
				}
				errno = EINVAL;						//invalid value, sets errno = EINVAL
				return -1;
			}
			else{
				conf->storageSize = value;
				i++;
				continue;
			}
		}
		if(strncmp(buf, STORAGEF, strlen(STORAGEF)) == 0){
			value = strtol(buf + strlen(STORAGEF), NULL, 10); 		//strtol of value, base 10
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				if(fclose(config) != 0){				//fclose sets errno
					return -1;
				}
				errno = EINVAL;						//invalid value, sets errno = EINVAL
				return -1;
			}
			else{
				conf->storageFileNumber = value;
				i++;
				continue;
			}
		}
		if(strncmp(buf, SOCKET, strlen(SOCKET)) == 0){
			strncpy(conf->pathToSocket, buf+strlen(SOCKET), MAX_SOCKET_LENGHT);
			conf->pathToSocket[strcspn(conf->pathToSocket, "\n")] = '\0';
			i++; 		
			continue;
		}
		if(strncmp(buf, REPOLICY, strlen(REPOLICY)) == 0){
			value = strtol(buf + strlen(REPOLICY), NULL, 10); 		//strtol of value, base 10
			//strtol may set errno for underflow or overflow!
			if(errno == ERANGE){
				if(fclose(config) != 0){				//fclose sets errno
					return -1;
				}
				errno = EINVAL;						//invalid value, sets errno = EINVAL
				return -1;
			}
			else{
				conf->repPolicy = value;
				i++;
				continue;
			}
		}
		
		if(strncmp(buf, LOGFILE, strlen(LOGFILE)) == 0){
			strncpy(conf->logPath, buf+strlen(LOGFILE), BUFFERSIZE);
			conf->logPath[strcspn(conf->logPath, "\n")] = '\0';
			i++;
			continue;
		}
	}
	if(fclose(config) != 0){							//fclose sets errno
		return -1;
	}
	return 0;

}
