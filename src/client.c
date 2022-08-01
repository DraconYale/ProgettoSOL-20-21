//#include <interface.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUFFERSIZE 1024
#define MAXLEN 256

#define HELP \
"Usage: ./client -f <filename> [OPTIONS]\n"\
"-h :				show help message\n"\
"-f <filename> :			connect to socket <filename>\n"\
"-w <dirname>[,n=0] :		sends to server a write request for n files in <dirname> and its subdirectories\n"\
"-W <file1>[,file2,...] :	sends to server a write request for <file1>[,file2,...]\n"\
"-D <dirname> :			expelled files from storage are saved in the directory 'dirname'\n"\
"-r <file1>[,file2,...] :	sends to server a read request for <file1>[,file2,...]\n"\
"-R [n=0] :			sends to server a read request for <n> files. If n=0 or it's not specified then reads all server files\n"\
"-d <dirname> :			files read with <-r> and <-R> options are stored in <dirname>\n"\
"-t <time> :			waits <time> milliseconds for the next request\n"\
"-l <file1>[,file2,...] :	lock request for <file1>[,file2,...]\n"\
"-u <file1>[,file2,...] :	unlock request for <file1>[,file2,...]\n"\
"-c <file1>[,file2,...] :	remove request for <file1>[,file2,...]\n"\
"-p :				enables output on stdout\n"


long isNumber(const char* s) {
   char* e = NULL;
   long val = strtol(s, &e, 0);
   if (e != NULL && *e == (char)0) return val; 
   return -1;
}

//validator checks if str is an option. Returns 0 if str is equal to one of the options
int validator(char* str){
	return ((strncmp(str, "-h", 4)) && (strncmp(str, "-f", 4)) && (strncmp(str, "-w", 4)) && \
	(strncmp(str, "-W", 4)) && (strncmp(str, "-D", 4)) && (strncmp(str, "-r", 4)) && \
	(strncmp(str, "-R", 4)) && (strncmp(str, "-d", 4)) && (strncmp(str, "-t", 4)) && \
	(strncmp(str, "-l", 4)) && (strncmp(str, "-u", 4)) && (strncmp(str, "-c", 4)) && \
	(strncmp(str, "-p", 4)));
}

int setHelp = 0;
int setDirWrite = 0;
int setDirRead = 0;
int time = 0;
int absTime = 5000;
int setPrint = 0;
int connected = 0;

int main(int argc, char** argv){
	if(argc < 2){
		printf("Too few arguments\n");
		fprintf(stdout, HELP);
		return 1;
	
	}
	int i = 1;
	char* dirWrite = calloc(MAXLEN, sizeof(char));
	char* dirRead = calloc(MAXLEN, sizeof(char));
	//"Preprocessing" for -h, -D, -d, -t and -p options
	int tmpTime = 0;
	char* tmp = NULL;
	char* tmp2 = NULL;
	while(i < argc && !setHelp){
		if((tmp = strrchr(argv[i], '-')) != NULL){
			if((strncmp(tmp, "-h", 2)) == 0){
				setHelp = 1;
			}
			if((strncmp(tmp, "-D", 2)) == 0){
				if(argv[i+1] != NULL && ((tmp2 = strrchr(argv[i+1], '-')) == NULL || validator(tmp2))){
					strncpy(dirWrite, argv[i+1], MAXLEN);
					setDirWrite = 1;
				}
				else{
					errno = EINVAL;
					perror("-D");
					free(dirWrite);
					free(dirRead);
					return -1;
				}
			}
			if((strncmp(tmp, "-d", 2)) == 0){
				if(argv[i+1] != NULL && ((tmp2 = strrchr(argv[i+1], '-')) == NULL || validator(tmp2))){
					strncpy(dirRead, argv[i+1], MAXLEN);
					setDirRead = 1;
				}
				else{
					errno = EINVAL;
					perror("-d");
					free(dirWrite);
					free(dirRead);
					return -1;
				}
			}
			if((strncmp(tmp, "-t", 2)) == 0){
				if((tmpTime = isNumber(argv[i+1])) != -1){
					time = tmpTime;
				}
				else{
					errno = EINVAL;
					perror("-t");
					free(dirWrite);
					free(dirRead);
					return -1;
				}
			}
			if((strncmp(tmp, "-p", 2)) == 0){
				setPrint = 1;
			}
		}
		i++;
	}
	if(setHelp){
		fprintf(stdout, HELP);
		free(dirWrite);
		free(dirRead);
		return 0;
	}
	char* socket = NULL;
	int opt;
	while((opt = getopt(argc, argv, "f:w::W:r:R::l:u:c:p")) != -1){
		switch(opt){
		    case 'f' :
		    	socket = optarg;
		       	if(openConnection(socket, time, absTime) != 0){
		       		perror("-f");
		       		return -1;
		       	}
		       	connected = 1;
		        break;
		    case 'w' :
		    	
		    	
		    	if(!setDirWrite){
		    		errno = ENOENT;
		    		perror("On -w:");
		    		return -1;            	
		    	}
		        
		    case 'W' :
		    	if(!setDirWrite){
		    		errno = ENOENT;
		    		perror("On -W:");
		    		return -1;            	
		    	}
		    
		    case 'D' :
		        
		    case 'r' :
		    	if(!setDirRead){
		    		errno = ENOENT;
		    		perror("On -r:");
		    		return -1;            	
		    	}
		        
		    case 'R' :
		    	if(!setDirRead){
		    		errno = ENOENT;
		    		perror("On -R:");
		    		return -1;            	
		    	}
		   
	
		    case 'l' :
		    
		    case 'u' :
		    
		    case 'c' :
		    
		    case 'p' :
		        
                
                
        	}
   	 }
	return 0;
}
