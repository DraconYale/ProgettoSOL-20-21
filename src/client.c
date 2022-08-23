#define _GNU_SOURCE
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <interface.h>
#include <functions.h>

#define BUFFERSIZE 1024
#define MAXLEN 256
#define UNIX_PATH_MAX 108

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
	if (e != NULL && *e == (char)0){
		return val;
	} 
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
int nWrite = 0;
int setDirRead = 0;
int timeC = 0;
int connected = 0;
char* filename = NULL;

//function used to visit recursively directories and send to server the files stored in them
int writeRecDir(char* dirname, char* dirMiss) {
	if(dirname == NULL){
		return -1;
	}
        
        struct stat statBuf;
	DIR * dir;

	if ((dir=opendir(dirname)) == NULL) {
		perror("opendir");
		return -1;
	} 
	else {
		struct dirent* elem;

		while((errno=0, elem =readdir(dir)) != NULL && nWrite != 0) {
			int parentL = strlen(dirname);
			int elemL = strlen(elem->d_name);
			if ((parentL+elemL+2)>UNIX_PATH_MAX) {
				errno = ENAMETOOLONG;	//ENAMETOOLONG 36 Nome del file troppo lungo (from "errno -l")
				perror("readdir");
				return -1;
			}
				
			char file[UNIX_PATH_MAX];
			strncpy(file,dirname,UNIX_PATH_MAX-1);
				
			if(dirname[strlen(dirname)-1] != '/'){
				strncat(file,"/",UNIX_PATH_MAX-1);
			}
			strncat(file,elem->d_name, UNIX_PATH_MAX-1);
			
			if(stat(file, &statBuf) == -1) {
				return -1;
			}
			if(S_ISDIR(statBuf.st_mode) && (strcmp(elem->d_name, ".") != 0)  && (strcmp(elem->d_name, "..") != 0)){
				writeRecDir(file, dirMiss);
			}
			else{
				if (openFile(file, O_LOCK | O_CREATE) != 0){
					perror("-w openFile");
					return -1;
				}
			
				if(writeFile(file, dirMiss) != 0){
					perror("-w writeFile");
					return -1;
				}
				if(closeFile(file) != 0){
					perror("-w closeFile");
					return -1;
				}
				nWrite--;
				sleep(timeC);
			}
		}
		if (errno != 0){
			perror("readdir");
		}
		closedir(dir);
    	}
	return 0;
}

int main(int argc, char** argv){
	if(argc < 2){
		printf("Too few arguments\n");
		fprintf(stdout, HELP);
		return 1;
	
	}
	struct timespec absTime;
	absTime.tv_nsec = 0;
	absTime.tv_sec = 10;
	setPrint = false;
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
					timeC = tmpTime/1000;			//convert to milliseconds
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
				setPrint = true;
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
	if(!setDirWrite){
		dirWrite = NULL;
	}
	if(!setDirRead){
		dirRead = NULL;
	}
	char* socket = NULL;
	int opt;
	int err;
	while((opt = getopt(argc, argv, "f:w:W:r:R::l:u:c:")) != -1){
		switch(opt){
		    case 'f' :
		    	if(!connected){
			    	socket = optarg;
			       	if(openConnection(socket, timeC, absTime) != 0){
			       		perror("-f");
			       		return -1;
			       	}
			       	connected = 1;
			       	sleep(timeC);
			       	break;
		       	}
		       	else{
		       		errno = EISCONN;		//EISCONN 106 Il socket di destinazione è già connesso (from "errno -l")
		       		perror("-f");
		       		break;
		       	}
		        
		        
		    case 'w' :
		    	if(connected){
		    		//tokenize optarg
		    		char* dir = NULL;
		    		char* nString = NULL;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		if(token != NULL){
		    			dir = token;
		    		}
		    		else{
		    			errno = EINVAL;
		    			perror("-w dirname");
		    			break;
		    		}
		    		token = strtok_r(NULL, " ", &strtokState);
		    		if(token != NULL){
		    			if(strncmp(token, "n=", 2)){
		    				if((nString = strrchr(token, '=')) != NULL){
		    					if((nWrite = (isNumber(nString+1))) != -1){
		    						continue;
		    					}
		    					else{
		    						errno = EINVAL;
		    						perror("n value in -w");
		    						break;
		    					}
		    				}
		    				else{
		    					errno = EINVAL;
		    					perror("-w n");
		    					break;
		    				}
		    			}
		    			else{
		    				errno = EINVAL;
		    				perror("n argument in -w");
		    			}
		    		}
		    		if(nWrite == 0){
		    			nWrite = -1;
		    		}
		    		if(writeRecDir(dir, dirWrite) != 0){
		    			return -1;
		    		}
		    		sleep(timeC);
		    		break;
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-w");
		    		break;
		    	}
		    	
		    	
		        
		    case 'W' :
		    	if(connected){
		    		//tokenize optarg
		    		char** files = calloc(MAXLEN, sizeof(char*));
		 		int filesNumber = 0;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		if(token == NULL){
		    			errno = EINVAL;
		    			perror("-W filename");
		    			break;
		    		
		    		}
		    		while(token != NULL){
		    			files[filesNumber] = token;
		    			filesNumber++;
		    			token = strtok_r(NULL, ",", &strtokState);
		    		}
		    		int w = 0;						//index used for pointing the files
		    		while(w<filesNumber){
		    			if((err = openFile(files[w], O_LOCK | O_CREATE)) != 0){
		    				perror("-W opening file");
		    			}
		    			if((err = writeFile(files[w],dirWrite)) != 0){
		    					perror("-W write");
		    			}
		    			w++;
		    		}
		    		free(files);
		    		sleep(timeC);
		    		break;
		    		
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-W");
		    		break;
		    	}
		    	
		    	
		        
		    case 'r' :
		    	if(connected){
		    		//tokenize optarg
		    		char** files = calloc(MAXLEN, sizeof(char*));
		 		int filesNumber = 0;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		while(token != NULL){
		    			files[filesNumber] = token;
		    			filesNumber++;
		    			token = strtok_r(NULL, ",", &strtokState);
		    		}
		    		int r = 0;						//index used for pointing the files
		    		while(r<filesNumber){
		    			if((err = openFile(files[r], O_LOCK | O_CREATE)) != 0){
		    				perror("-r opening file");
		    			}
		    			char* ansBuf = NULL;
		    			size_t readSize = 0;
		    			if((err = readFile(files[r], (void**) &ansBuf,&readSize)) != 0){
		    					perror("-r read file");
		    					break;
		    			}
		    			if(dirRead != NULL){
		    				char file[UNIX_PATH_MAX];
						strncpy(file,dirRead,UNIX_PATH_MAX-1);
						strncat(file,files[r], UNIX_PATH_MAX-1);
						FILE* dest;
						if((dest = fopen(file, "w")) == NULL){
							perror("-r opening dest file");
							free(files);
							break;
						}
						if(fwrite(ansBuf, readSize, 1, dest) != 1){
							perror("-r writing in dest file");
							free(files);
							break;
						}
						fclose(dest);
		    			}
		    			r++;
		    		}
		    		free(files);
		    		sleep(timeC);
		    		break;
		    		
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-r");
		    		break;
		    	}
		    	
		        
		    case 'R' :
		    	if(connected){
		    		int n = 0;
		    		if(optarg == 0){
		    			if((err = (readNFiles(0,dirRead))) != 0){
		    				perror("-R read");
		    			}
		    		}
		    		else{   	
		    			char* nString = NULL;		
		    			if(strncmp(optarg, "n=", 2)){
		    				if((nString = strrchr(optarg, '=')) != NULL){
		    					if((n = (isNumber(nString+1))) != -1){
		    						if((err = (readNFiles(n,dirRead))) != 0){
		    							perror("-R read");
		    						}
		    					}
		    					else{
		    						errno = EINVAL;
		    						perror("n value in -R");
		    						break;
		    					}
		    				}
		    				else{
		    					errno = EINVAL;
		    					perror("-R n");
		    					break;
		    				}
		    			}
		    			else{
		    				errno = EINVAL;
		    				perror("n argument in -R");
		    			}
		    		
		    		}
		    		sleep(timeC);
		    		break;
		    		
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-R");
		    		break;
		    	}
		    	
		   
	
		    case 'l' :
		    	if(connected){
		    		//tokenize optarg
		    		char** files = calloc(MAXLEN, sizeof(char*));
		 		int filesNumber = 0;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		while(token != NULL){
		    			files[filesNumber] = token;
		    			filesNumber++;
		    			token = strtok_r(NULL, ",", &strtokState);
		    		}
		    		int l = 0;						//index used for pointing the files
		    		while(l<filesNumber){
		    			if((err = openFile((files[l]), O_LOCK | O_CREATE) != 0)){
		    				perror("-l opening file");
		    			}
		    			if((err = lockFile(files[l])) != 0){
		    					perror("-l lock");
		    			}
		    			l++;
		    		}
		    		free(files);
		    		sleep(timeC);
		    		break;
		    		
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-l");
		    		break;
		    	}
		    	
		    case 'u' :
		    	if(connected){
		    		//tokenize optarg
		    		char** files = calloc(MAXLEN, sizeof(char*));
		 		int filesNumber = 0;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		while(token != NULL){
		    			files[filesNumber] = token;
		    			filesNumber++;
		    			token = strtok_r(NULL, ",", &strtokState);
		    		}
		    		int u = 0;						//index used for pointing the files
		    		while(u<filesNumber){
		    			if((err = openFile((files[u]), O_LOCK | O_CREATE) != 0)){
		    				perror("-u opening file");
		    			}
		    			if((err = unlockFile(files[u])) != 0){
		    					perror("-u unlock");
		    			}
		    			u++;
		    		}
		    		free(files);
		    		sleep(timeC);
		    		break;
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-u");
		    		break;
		    	}
		    	
		    case 'c' :
		    	if(connected){
		    		//tokenize optarg
		    		char** files = calloc(MAXLEN, sizeof(char*));
		 		int filesNumber = 0;
		    		char* strtokState = NULL;
		    		char* token = strtok_r(optarg, ",", &strtokState);
		    		while(token != NULL){
		    			files[filesNumber] = token;
		    			filesNumber++;
		    			token = strtok_r(NULL, ",", &strtokState);
		    		}
		    		int c = 0;						//index used for pointing the files
		    		while(c<filesNumber){
		    			if((err = openFile((files[c]), O_LOCK | O_CREATE) != 0)){
		    				perror("-c opening file");
		    			}
		    			if((err = removeFile(files[c])) != 0){
		    					perror("-c remove");
		    			}
		    			c++;
		    		}
		    		free(files);
		    		sleep(timeC);
		    		break;
		    	
		    	}
		    	else{
		    		errno = ENOTCONN; 	//ENOTCONN 107 Il socket di destinazione non è connesso (from "errno -l")
				perror("-c");
		    		break;
		    	}
        	}
	}
	if(dirWrite != NULL){
		free(dirWrite);
	}
	if(dirRead != NULL){
		free(dirRead);
	}
	if(closeConnection(socket) != 0){
		perror("closeConnection");
		return -1;
	}
	return 0;
}
