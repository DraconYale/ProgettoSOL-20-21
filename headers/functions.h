#ifndef FUNCTIONS_H_DEFINED
#define FUNCTIONS_H_DEFINED

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#define O_CREATE 1
#define O_LOCK 2

//opcodes
typedef enum operations{
	OPEN,
	CLOSE,
	READ,
	READN,
	WRITE,
	APPEND,
	LOCK,
	UNLOCK,
	REMOVE,
	CLOSECONN
} operation;

static inline ssize_t  /* Read "n" bytes from a descriptor */
readn(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nread;
   
   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}
 
static inline ssize_t  /* Write "n" bytes to a descriptor */
writen(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nwritten;
 
   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}

static inline int mkdirs(char* path){
	
	char* pointer;
	char* addPath = malloc(strlen(path)*sizeof(char)+1);
	strcpy(addPath, path);
	pointer = strrchr(addPath, '/');
	if(pointer != NULL){
		*pointer = '\0';
	}
	char* p;
	for(p = addPath+1; *p; p++){
		if(*p == '/'){
					
			*p = '\0';
			if(mkdir(addPath, S_IRWXU) != 0){
				if(errno != EEXIST){
					return -1;
				}
			}
			*p = '/';
		}
					
				
	}
	if(mkdir(addPath, S_IRWXU) != 0){
		if(errno != EEXIST){
			return -1;
		}
	}
	free(addPath);
	return 0;


}

#endif
