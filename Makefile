CC = gcc
CFLAGS += -std=c99 -Wall -g 
HEADERS = -I ./headers/

all:	bin/server bin/client

.PHONY: all clean cleanall test1 test2 test3

SERVER_OBJ = obj/boundedBuffer.o obj/icl_hash.o obj/list.o obj/locker.o obj/storage.o obj/txtParser.o obj/server.o
CLIENT_OBJ = obj/interface.o obj/client.o

obj/boundedBuffer.o:	src/boundedBuffer.c
	$(CC) $(CFLAGS) $(HEADERS) -c $^ -lpthread -o $@
	
obj/icl_hash.o:	src/icl_hash.c
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

test1:	bin/server bin/client
	@chmod +x script/test1.sh
	script/test1.sh

test2:	bin/server bin/client
	@chmod +x script/test2.sh
	script/test2.sh

test3:	bin/server bin/client
	@chmod +x script/test3.sh
	@chmod +x script/test3Clients.sh
	script/test3.sh

clean cleanall: 
	rm -rf bin/* obj/* test1 test2 test3
	rm log/test1/*.txt
	rm log/test2/*.txt
	rm log/test3/*.txt
	@touch bin/.keep
	@touch obj/.keep
	@touch log/test1/.keep
	@touch log/test2/.keep
	@touch log/test3/.keep
