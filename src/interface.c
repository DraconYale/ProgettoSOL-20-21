#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <interface.h>
#include <functions.h>


#define MAXLEN 256
#define UNIX_PATH_MAX 108
#define COMMLENGTH 1024

static int csfd;							//client socket file descriptor
static char cSockname[UNIX_PATH_MAX];
bool setPrint = false;
char requestBuf[MAXLEN];
char retString[MAXLEN];
char errorString[COMMLENGTH];
int retCode;

#define PRINT(cond, ...) \
	if(cond){ \
		fprintf(stdout, __VA_ARGS__); \
	}

/*
openConnection: opens an AF_UNIX connection to socket 'sockname'. If the connection is not accepted immediately, the client connection is
		repeated after 'msec' milliseconds. After 'abstime' connection attempts are not possible anymore.
		Returns (0) if success, (-1) if failure and sets errno.
*/
int openConnection(const char* sockname, int msec, const struct timespec abstime){

	if(sockname == NULL || strlen(sockname) > UNIX_PATH_MAX || msec < 0){
		errno = EINVAL;
		PRINT(setPrint, "openConnection to %s: fail with error %d\n", sockname, errno);
		return -1;
	}
	if((csfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		PRINT(setPrint, "openConnection to %s: fail with error %d\n", sockname, errno);
		return -1;
	}
	struct sockaddr_un sock_addr;
	strncpy(sock_addr.sun_path, sockname, strlen(sockname) + 1);
	sock_addr.sun_family = AF_UNIX;
	while((connect(csfd, (struct sockaddr*) &sock_addr, sizeof(sock_addr))) == -1){
		if (errno != ENOENT){					//ENOENT 2 File o directory non esistente
			PRINT(setPrint, "openConnection to %s: fail with error %d\n", sockname, errno);
			return -1;
		}
		time_t current = time(NULL);				//current time
		if (current > abstime.tv_sec){		
			errno = EAGAIN;					//EAGAIN 11 Risorsa temporaneamente non disponibile
			PRINT(setPrint, "openConnection to %s: fail with error %d\n", sockname, errno);		
			return -1;
		}
		sleep(msec); 
		errno = 0;
	}
	strcpy(cSockname, sockname);
	PRINT(setPrint, "openConnection to %s: OK\n", sockname);
	return 0;
}


/*
closeConnection: closes the AF_UNIX connection to socket 'sockname'.
		 Returns (0) if success, (-1) if failure and sets errno.
*/
int closeConnection(const char* sockname){

	if (sockname == NULL || strlen(sockname) > UNIX_PATH_MAX || strncmp(sockname, cSockname, UNIX_PATH_MAX) != 0){
		errno = EINVAL;
		PRINT(setPrint, "closeConnection to %s: fail with error %d\n", sockname, errno);
		return -1;
	}
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d", CLOSECONN);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "closeConnection %s: fail with error %d\n", sockname, errno);
		return -1;
	}
	if (close(csfd) != 0){
		PRINT(setPrint, "closeConnection to %s: fail with error %d\n", sockname, errno);
		return -1;
	}
	csfd = -1;
	free(tmpBuf);
	PRINT(setPrint, "closeConnection to %s: OK\n", sockname);
	return 0;
}


/*
openFile: open or creation request for file 'pathname'. Possible 'flags' are O_CREATE and O_LOCK (it's possible to OR them)
	  Returns (0) if success, (-1) if failure and sets errno.
*/
int openFile(const char* pathname, int flags){

	if (pathname == NULL || strlen(pathname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	int error;
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s %d", OPEN, pathname, flags);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "openFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "openFile %s: fatal error %d\n", pathname, errno);
			exit(EXIT_FAILURE);
	
	}
	free(tmpBuf);
	PRINT(setPrint, "openFile %s: OK\n", pathname);
	return 0;
	
}


/*
readFile: reads all the contents in file 'pathname' and returns a pointer to heap memory in 'buf' and the buffer size in 'size'.
	  Returns (0) if success, (-1) if failure (i.e. 'pathname' doesn't exist) and sets errno.
*/
int readFile(const char* pathname, void** buf, size_t* size){

	if(pathname == NULL || buf == NULL){
		errno = EINVAL;
		PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	int error;
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s", READ, pathname);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}

	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
			exit(EXIT_FAILURE);
	
	}
	char* readBuf = NULL;
	unsigned long sizeBuf = 0;
	char msg[MAXLEN];			
	memset(msg, 0, 32);
	if(readn(csfd, (void*)msg, MAXLEN) == -1){
		PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	sscanf(msg, "%lu", &sizeBuf);
	if(size != 0){
		if((readBuf = calloc(sizeBuf+1, sizeof(char))) == NULL){
			PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
			return -1;
		}
		if(readn(csfd, (void*)readBuf, sizeBuf) == -1){
			PRINT(setPrint, "readFile %s: fail with error %d\n", pathname, errno);
			return -1;
		}
		readBuf[sizeBuf+1] = '\0';
	
	}
	*buf = (void*) readBuf;
	*size = sizeBuf;
	free(tmpBuf);
	PRINT(setPrint, "readFile %s: OK. Read size: %lu\n", pathname, sizeBuf);
	return 0;
}


/*
readNFiles: client request for N files. If server file count is <N or N<=0 then it sends all the files to the client.
	    Returns (0) if success, (-1) if failure and sets errno.
*/
int readNFiles(int N, const char* dirname){

	if(dirname != NULL && strlen(dirname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
		return -1;
	}
	
	int error = 0;
	unsigned long writtenBytes = 0;
	unsigned long readBytes = 0;
	
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, MAXLEN, "%d %d", READN, N);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		return -1;
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "readNFile %d: fail with error %d\n", N, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "readNFile %d: fail with error %d\n", N, errno);
			free(tmpBuf);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "readNFile %d: fail with error %d\n", N, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "readNFile %d: fatal error %d\n", N, error);
			free(tmpBuf);
			exit(EXIT_FAILURE);
	
	}
	char readNumberStr[MAXLEN];
	memset(readNumberStr, 0, MAXLEN);
	int readNumber = 0;
	if (readn(csfd, (void*) readNumberStr, MAXLEN) == -1){
		PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
		return -1;
	}
	sscanf(readNumberStr, "%d", &readNumber);
	int j = 0;
	char name[MAXLEN];
	char msg[MAXLEN];
	int nameLeng = 0;
	unsigned long sizeBuf = 0;
	char* readCont = NULL;
	while(j < readNumber){
	
			sizeBuf = 0;
			//name length
			memset(msg, 0, MAXLEN);
			
			if (readn(csfd, (void*) msg, MAXLEN) <= 0){
				PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
				return -1;
			}
			
			sscanf(msg, "%d", &nameLeng);
			memset(name, 0, MAXLEN);
			if (readn(csfd, (void*) name, MAXLEN) == -1){
				PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
				return -1;
			}
			memset(msg, 0, 32);
			if (readn(csfd, (void*) msg, MAXLEN) == -1){
				PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
				return -1;
			}
			sscanf(msg, "%lu", &sizeBuf);
			if(sizeBuf != 0){
				if((readCont = calloc(sizeBuf, sizeof(char))) == NULL){
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}
				//file content
				if (readn(csfd, (void*) readCont, sizeBuf) <= 0){
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}
			
			}
			readBytes = readBytes + sizeBuf;
			if(dirname != NULL){
				char tmpPath[UNIX_PATH_MAX];
				strcpy(tmpPath, dirname);
				if((strlen(dirname)+(strlen(name)) > UNIX_PATH_MAX)){
					errno = ENAMETOOLONG;
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}
				char newPath[UNIX_PATH_MAX];
				strcpy(newPath, tmpPath);
				strncat(newPath, name, strlen(name)+1);
				//create directories for saving files
				mkdirs(newPath);
				writtenBytes = writtenBytes + sizeBuf;
		
				FILE* savedFile;
				if((savedFile = fopen(newPath, "w+")) == NULL){
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}
				if((fwrite(readCont, 1, sizeBuf, savedFile)) != sizeBuf){
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}
				if(fclose(savedFile) != 0){
					PRINT(setPrint, "readNFile %d %s: fail with error %d\n", N, dirname, errno);
					return -1;
				}		
			}
			free(readCont);
			readCont = NULL;
			sizeBuf = 0;
			j++;
	
	}
	free(tmpBuf);
	PRINT(setPrint, "readNFile %d %s: OK. Read files: %d Read bytes: %lu Written bytes: %lu\n", N, dirname, readNumber , readBytes, writtenBytes);
	return 0;
}


/*
writeFile: writes the 'pathname' file in the server. Server may expel one file to store 'pathname' file. If this is the case: if 'dirname' is
	   not NULL, the expelled file from server is saved in 'dirname', else the expelled file is destroyed.
	   Returns (0) if success, (-1) if failure and sets errno.
	   
*/
int writeFile(const char* pathname, const char* dirname){

	if(pathname == NULL){
		errno = EINVAL;
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	if(dirname != NULL){
		if(strlen(dirname) > UNIX_PATH_MAX){
			errno = EINVAL;
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			return -1;
		}
	
	}
	int error;
	unsigned long writtenBytes = 0;
	unsigned long readBytes = 0;
	
	
	//we need to calculate the file's size
	unsigned long fileSize = 0;
	FILE* file;
	if((file  = fopen(pathname, "r")) == NULL){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;	
	}
	if((fseek(file, 0, SEEK_END)) != 0){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	
	fileSize = ftell(file);
	if (fseek(file, 0, SEEK_SET) != 0){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	char* fileContent = NULL;
	if (fileSize != 0){
		if((fileContent = malloc(sizeof(char) * (fileSize + 1))) == NULL){
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			fclose(file);
			return -1;
		}
		unsigned long readSize; 
		if ((readSize = fread(fileContent, sizeof(char), fileSize, file)) != fileSize){
			fclose(file);
			free(fileContent);
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			return -1;
		}
		fileContent[fileSize] = '\0';
	}
	fclose(file);

	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s %lu", WRITE, pathname, fileSize);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "writeFile %s %lu: fail with error %d\n", pathname, fileSize, errno);
		return -1;
	}
	if (fileSize != 0){
		if (writen(csfd, (void*) fileContent, fileSize) == -1){
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			free(fileContent);
			return -1;
		}
		free(fileContent);
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			writtenBytes = writtenBytes + fileSize;
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
			exit(EXIT_FAILURE);
	
	}
	char readNumberStr[MAXLEN];
	int victimNumber = 0;
	if (readn(csfd, (void*) readNumberStr, MAXLEN) == -1){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	sscanf(readNumberStr, "%d", &victimNumber);
	int j = 0;
	char name[MAXLEN];
	char msg[MAXLEN];
	char msgSize[MAXLEN];
	int nameLeng = 0;
	unsigned long sizeBuf = 0;
	char* readCont = NULL;
	while(j < victimNumber){
			sizeBuf = 0;
			//name length
			memset(msg, 0, MAXLEN);
			if (readn(csfd, (void*) msg, MAXLEN) <= 0){
				PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			sscanf(msg, "%d", &nameLeng);
			
			//file name
			memset(name, 0, MAXLEN);
			if (readn(csfd, (void*) name, MAXLEN) <= 0){
				PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			
			//file size
			memset(msgSize, 0, MAXLEN);
			if(readn(csfd, (void*) msgSize, MAXLEN) <= 0){
				PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			sscanf(msgSize, "%lu", &sizeBuf);
			readBytes = readBytes + sizeBuf;
			if(sizeBuf != 0){
				if((readCont = malloc(sizeBuf*sizeof(char)+1)) == NULL){
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				//file content
				if (readn(csfd, (void*) readCont, sizeBuf) <= 0){
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
			
			}
			if(dirname != NULL){
			
				char tmpPath[UNIX_PATH_MAX];
				strcpy(tmpPath, dirname);
				if((strlen(dirname)+(strlen(name)) > UNIX_PATH_MAX)){
					errno = ENAMETOOLONG;
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				char newPath[UNIX_PATH_MAX];
				strcpy(newPath, tmpPath);
				strncat(newPath, name, strlen(name)+1);
				//create directories for saving files
				mkdirs(newPath);
		
				FILE* savedFile;
				if((savedFile = fopen(newPath, "w+")) == NULL){
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				if((fwrite(readCont, 1, sizeBuf, savedFile)) != sizeBuf){
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				if(fclose(savedFile) != 0){
					PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
			}
			free(readCont);
			readCont = NULL;
			sizeBuf = 0;
			j++;
	
	}
	free(tmpBuf);
	PRINT(setPrint, "writeFile %s %s: OK. Victim files from server: %d Read bytes: %lu Written bytes: %lu\n", pathname, dirname, victimNumber , readBytes, writtenBytes);
	return 0;
}

/*
appendToFile: append to 'pathname' 'size' bytes from buffer 'buf'. The append operation is atomic. Server may expel one file to store 'pathname'
	      file. If this is the case: if 'dirname' is not NULL, the expelled file from server is saved in 'dirname', else the expelled 
	      file is destroyed.
	      Returns (0) if success, (-1) if failure and sets errno. 
*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
	
	if(pathname == NULL || strlen(dirname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	int error;
	unsigned long writtenBytes = 0;
	unsigned long readBytes = 0;
	
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s %lu", APPEND, pathname, size);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	
	if (size != 0){
		if (writen(csfd, (void*) buf, size) == -1){
			PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
			return -1;
		}
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "appendToFile %s %s: fatal error %d\n", pathname, dirname, errno);
			exit(EXIT_FAILURE);
	
	}
	char readNumberStr[MAXLEN];
	int victimNumber = 0;
	if (readn(csfd, (void*) readNumberStr, MAXLEN) == -1){
		PRINT(setPrint, "writeFile %s %s: fail with error %d\n", pathname, dirname, errno);
		return -1;
	}
	sscanf(readNumberStr, "%d", &victimNumber);
	int j = 0;
	char name[MAXLEN];
	char msg[MAXLEN];
	char msgSize[MAXLEN];
	int nameLeng = 0;
	unsigned long sizeBuf = 0;
	char* readCont = NULL;
	while(j < victimNumber){
			sizeBuf = 0;
			//name length
			memset(msg, 0, MAXLEN);
			if (readn(csfd, (void*) msg, MAXLEN) <= 0){
				PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			sscanf(msg, "%d", &nameLeng);
			
			//file name
			memset(name, 0, MAXLEN);
			if (readn(csfd, (void*) name, MAXLEN) <= 0){
				PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			//file size
			memset(msgSize, 0, MAXLEN);
			if(readn(csfd, (void*) msgSize, MAXLEN) <= 0){
				PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
				return -1;
			}
			
			sscanf(msgSize, "%lu", &sizeBuf);
			readBytes = readBytes + sizeBuf;
			if(sizeBuf != 0){
				if((readCont = malloc(sizeBuf*sizeof(char)+1)) == NULL){
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				//file content
				if (readn(csfd, (void*) readCont, sizeBuf) <= 0){
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
			
			}
			if(dirname != NULL){
				char tmpPath[UNIX_PATH_MAX];
				strcpy(tmpPath, dirname);
				if((strlen(dirname)+(strlen(name)) > UNIX_PATH_MAX)){
					errno = ENAMETOOLONG;
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				char newPath[UNIX_PATH_MAX];
				strcpy(newPath, tmpPath);
				strncat(newPath, name, strlen(name)+1);
				//create directories for saving files
				mkdirs(newPath);

		
				FILE* savedFile;
				if((savedFile = fopen(newPath, "w+")) == NULL){
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				if((fwrite(readCont, 1, sizeBuf, savedFile)) != sizeBuf){
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}
				if(fclose(savedFile) != 0){
					PRINT(setPrint, "appendToFile %s %s: fail with error %d\n", pathname, dirname, errno);
					return -1;
				}	
			}
			free(readCont);
			readCont = NULL;
			sizeBuf = 0;
			j++;
	
	}
	free(tmpBuf);
	PRINT(setPrint, "appendToFile %s %s: OK. Victim files from server: %d Read bytes: %lu Written bytes: %lu\n", pathname, dirname, victimNumber , readBytes, writtenBytes);
	return 0;
}

/*
lockFile: sets the O_LOCK flag for file 'pathname'. If 'pathname' was opened/created with O_LOCK flag and the request is from the same process
	  or the file's O_LOCK flag is not set, then the operation returns success, else the operation waits that the O_LOCK flag is reset
	  by the lock owner.
	  Returns (0) if success, (-1) if failure and sets errno. 
*/
int lockFile(const char* pathname){
	
	if (pathname == NULL || strlen(pathname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	int error;
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	snprintf(tmpBuf, COMMLENGTH, "%d %s", LOCK, pathname);
	//lockFile waits until client gets the lock
	while(true){
		//request will be parsed by server
		if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
			PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
			return -1;
		}
		
		memset(retString, 0, MAXLEN);
		if(readn(csfd, (void*)retString, 2) == -1){
			PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
			return -1;
		}
		
		sscanf(retString, "%d", &retCode);
		memset(retString, 0, MAXLEN);
		switch(retCode){
			
			case 0:
				free(tmpBuf);
				PRINT(setPrint, "lockFile %s: OK\n", pathname);
				return 0;
			
			case -1:
				error = 0;
				if (readn(csfd, &error, sizeof(int)) == -1){
					PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
					return -1;
				}
				errno = error/256;
				if(errno != EPERM){
					PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
					free(tmpBuf);
					return -1;
				}
			case -2:
				error = 0;
				if (readn(csfd, &error, sizeof(int)) == -1){
					PRINT(setPrint, "lockFile %s: fail with error %d\n", pathname, errno);
					return -1;
				}
				errno = error/256;
				PRINT(setPrint, "lockFile %s: fatal error %d\n", pathname, errno);
				free(tmpBuf);
				exit(EXIT_FAILURE);
		
		}
	}
}

/*
unlockFile: resets the O_LOCK flag for file 'pathname'.
	    Returns (0) if success (lock owner calls unlockFile), (-1) failure (unlockFile is called by another process) and sets errno.
*/
int unlockFile(const char* pathname){
	
	if (pathname == NULL || strlen(pathname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	int error;
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s", UNLOCK, pathname);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "unlockFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "unlockFile %s: fatal error %d\n", pathname, errno);
			exit(EXIT_FAILURE);
	
	}
	free(tmpBuf);
	PRINT(setPrint, "unlockFile %s: OK\n", pathname);
	return 0;
}


/*
closeFile: close request of file 'pathname'.
	   Returns (0) if success, (-1) if failure and sets errno.
*/
int closeFile(const char* pathname){
	
	if (pathname == NULL || strlen(pathname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	int error;
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s", CLOSE, pathname);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "closeFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "closeFile %s: fatal error %d\n", pathname, errno);
			exit(EXIT_FAILURE);
	
	}
	free(tmpBuf);
	PRINT(setPrint, "closeFile %s: OK\n", pathname);
	return 0;
}


/*
removeFile: remove file 'pathname' from the server.
	    Returns (0) if success, (-1) if failure (file is not locked or is locked by another process) and sets errno.
*/
int removeFile(const char* pathname){
	
	if (pathname == NULL || strlen(pathname) > UNIX_PATH_MAX){
		errno = EINVAL;
		PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	char* tmpBuf = calloc(COMMLENGTH, sizeof(char));
	int error;
	//request will be parsed by server
	snprintf(tmpBuf, COMMLENGTH, "%d %s", REMOVE, pathname);
	if(writen(csfd, (void *)tmpBuf, COMMLENGTH) == -1){
		PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	memset(retString, 0, MAXLEN);
	if(readn(csfd, (void*)retString, 2) == -1){
		PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
		return -1;
	}
	
	sscanf(retString, "%d", &retCode);
	memset(retString, 0, MAXLEN);
	switch(retCode){
		
		case 0:
			break;
		
		case -1:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
			return -1;
		case -2:
			error = 0;
			if (readn(csfd, &error, sizeof(int)) == -1){
				PRINT(setPrint, "removeFile %s: fail with error %d\n", pathname, errno);
				return -1;
			}
			errno = error/256;
			PRINT(setPrint, "removeFile %s: fatal error %d\n", pathname, errno);
			exit(EXIT_FAILURE);
	
	}
	free(tmpBuf);
	PRINT(setPrint, "removeFile %s: OK\n", pathname);
	return 0;
}
