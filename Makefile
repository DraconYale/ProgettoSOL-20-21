CC = gcc
CFLAGS += -std=c99 -Wall -g
HEADERS = -I ./headers/

.PHONY: all clean cleanall test1 test2 test3

SERVER_OBJ = obj/boundedBuffer.o obj/hashtable.o obj/list.o obj/locker.o obj/storage.o obj/txtParser.o obj/server.o
CLIENT_OBJ = obj/interface.o obj/client.o

obj/boundedBuffer.o:	src/boundedBuffer.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/hashtable.o:	src/hashtable.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/interface.o:	src/interface.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@

obj/list.o:	src/list.c 
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@

obj/locker.o:	src/locker.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/txtParser.o:	src/txtParser.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/storage.o:	src/storage.c 
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/client.o:	src/client.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@

obj/server.o:	src/server.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@

bin/client:	$(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(CLIENT_OBJ) -lpthread -o $@

bin/server:	$(SERVER_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -lpthread -o $@

all:	bin/server bin/client

test1:

test2:

test3:

clean:

cleanall:
