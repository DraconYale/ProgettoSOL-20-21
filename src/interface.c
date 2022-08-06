#include <interface.h>

#include <stdio.h>
#include <stlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAXLEN 256
#define UNIX_PATH_MAX 108

int csfd;				//client socket file descriptor
char* cSockname= calloc(UNIX_PATH_MAX, sizeof(char));
/*
openConnection: opens an AF_UNIX connection to socket 'sockname'. If the connection is not accepted immediately, the client connection is
		repeated after 'msec' milliseconds. After 'abstime' connection attempts are not possible anymore.
		Returns (0) if success, (-1) if failure and sets errno.
*/
int openConnection(const char* sockname, int msec, const struct timespec abstime){
	if(sockname == NULL || strlen(sockname) > UNIX_PATH_MAX || msec < 0){
		errno = EINVAL;
		return -1;
	}
	if((csfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		return -1;
	}
	struct sockaddr_un sock_addr;
	strncpy(sock_addr.sun_path, sockname, strlen(sockname) + 1);
	sock_addr.sun_family = AF_UNIX;
	while((connect(csfd, &sock_addr, sizeof(sock_addr))) == -1){
		if (errno != ENOENT){					//ENOENT 2 File o directory non esistente
			return -1;
		}
		time_t current = time(NULL);				//current time
		if (current > abstime.tv_sec){		
			err = EAGAIN;					//EAGAIN 11 Risorsa temporaneamente non disponibile		
			return -1;
		}
		sleep(msec); 
		errno = 0;
	}
	strncpy(cSockname, sockname, UNIX_PATH_MAX);
	return 0;
}


/*
closeConnection: closes the AF_UNIX connection to socket 'sockname'.
		 Returns (0) if success, (-1) if failure and sets errno.
*/
int closeConnection(const char* sockname){
	if (sockname == NULL || strlen(sockname) > UNIX_PATH_MAX || strncmp(sockname, cSockname, UNIX_PATH_MAX) != 0){
		err = EINVAL;
		return -1;
	}
	if (close(csfd) != 0){
		return -1;
	}
	return 0;
}


/*
openFile: open or creation request for file 'pathname'. Possible 'flags' are O_CREATE and O_LOCK (it's possible to OR them)
	  Returns (0) if success, (-1) if failure and sets errno.
*/
int openFile(const char* pathname, int flags){
	//TODO
}


/*
readFile: reads all the contents in file 'pathname' and returns a pointer to heap memory in 'buf' and the buffer size in 'size'.
	  Returns (0) if success, (-1) if failure (i.e. 'pathname' doesn't exist) and sets errno.
*/
int readFile(const char* pathname, void** buf, size_t* size){
	//TODO
}


/*
readNFiles: client request for N files. If server file count is <N or N<=0 then it sends all the files to the client.
	    Returns (0) if success, (-1) if failure and sets errno.
*/
int readNFiles(int N, const char* dirname){
	//TODO
}


/*
writeFile: writes the 'pathname' file in the server. Server may expel one file to store 'pathname' file. If this is the case: if 'dirname' is
	   not NULL, the expelled file from server is saved in 'dirname', else the expelled file is destroyed.
	   Returns (0) if success, (-1) if failure and sets errno.
	   
*/
int writeFile(const char* pathname, const char* dirname){
	//TODO
}

/*
appendToFile: append to 'pathname' 'size' bytes from buffer 'buf'. The append operation is atomic. Server may expel one file to store 'pathname'
	      file. If this is the case: if 'dirname' is not NULL, the expelled file from server is saved in 'dirname', else the expelled 
	      file is destroyed.
	      Returns (0) if success, (-1) if failure and sets errno. 
*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
	//TODO
}

/*
lockFile: sets the O_LOCK flag for file 'pathname'. If 'pathname' was opened/created with O_LOCK flag and the request is from the same process
	  or the file's O_LOCK flag is not setted, then the operation returns success, else the operation waits that the O_LOCK flag is resetted
	  by the lock owner.
	  Returns (0) if success, (-1) if failure and sets errno. 
*/
int lockFile(const char* pathname){
	//TODO
}

/*
unlockFile: resets the O_LOCK flag for file 'pathname'.
	    Returns (0) if success (lock owner calls unlockFile), (-1) failure (unlockFile is called by another process) and sets errno.
*/
int unlockFile(const char* pathname){
	//TODO
}


/*
closeFile: close request of file 'pathname'.
	   Returns (0) if success, (-1) if failure and sets errno.
*/
int closeFile(const char* pathname){
	//TODO
}


/*
removeFile: remove file 'pathname' from the server.
	    Returns (0) if success, (-1) if failure (file is not locked or is locked by another process) and sets errno.
*/
int removeFile(const char* pathname){
	//TODO
}
