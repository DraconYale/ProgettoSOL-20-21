#!/bin/bash

echo -e "\n== TEST 1  =="

echo -e "\nCreating test files..."

mkdir files
touch files/1.txt
touch files/2.txt
touch files/3.txt
touch files/4.txt
touch files/5.txt
touch files/6.txt
touch files/7.txt
touch files/8.txt
touch files/9.txt
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/1.txt
head -c 2MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/2.txt
head -c 3MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/3.txt
head -c 5MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/4.txt
head -c 6MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/5.txt
head -c 7MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/6.txt
head -c 8MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/7.txt
head -c 8MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/8.txt
head -c 8MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files/9.txt

echo -e "\nTest files created"

echo -e "\n[FIFO] Starting server (with 'valgrind --leak-check=full')"
valgrind --leak-check=full bin/server config/test1/FIFOconfig.txt &

SERVER_PID=$!
export SERVER_PID

sleep 3s

echo -e "\n[FIFO] Starting clients"

#print usage
bin/client -h

#connect and then disconnect without doing anything
echo -e "\n[FIFO] First client"
bin/client -p -t 200 -f bin/socket.sk

#send files (victims are sent to specified directory) and unlock two files 
echo -e "\n[FIFO] Second client"
bin/client -p -t 200 -f bin/socket.sk -w files -D test1/FIFO/victims/ -u files/5.txt,files/6.txt

#send src files (victims are sent to specified directory) and unlock one file
echo -e "\n[FIFO] Third client"
bin/client -p -t 200 -f bin/socket.sk -w src -D test1/FIFO/victims/ -u src/server.c

#read some files (read files are sent to the specified directory) and delete one file
echo -e "\n[FIFO] Fourth client"
bin/client -p -t 200 -f bin/socket.sk -r files/5.txt,files/6.txt,src/server.c,src/client.c -d test1/FIFO/read/ -c src/server.c

#send one file, locks it and deletes it
echo -e "\n[FIFO] Fifth client"
bin/client -p -t 200 -f bin/socket.sk -W src/server.c -l src/server.c -c src/server.c

#tries to read all files from server (read files are sent to the specified directory)
echo -e "\n[FIFO] Sixth client"
bin/client -p -t 200 -f bin/socket.sk -R n=0 -d test1/FIFO/read/

echo -e "\n[FIFO] Client requests finished"

echo -e "\n[FIFO] Sending SIGHUP to server"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "\n[FIFO] FIFO test 1 completed"

echo -e "\n[LRU] Starting server (with 'valgrind --leak-check=full')"

valgrind --leak-check=full bin/server config/test1/LRUconfig.txt &

SERVER_PID=$!
export SERVER_PID

sleep 3s

echo -e "\n[LRU] Starting clients"

#print usage
bin/client -h

#connect and then disconnect without doing anything
echo -e "\n[LRU] First client"
bin/client -p -t 200 -f bin/socket.sk

#send files (victims are sent to specified directory) and unlock two files
echo -e "\n[LRU] Second client"
bin/client -p -t 200 -f bin/socket.sk -w files -D test1/LRU/victims/ -u files/3.txt,files/4.txt

#send src files (victims are sent to specified directory) and unlock one file
echo -e "\n[LRU] Third client"
bin/client -p -t 200 -f bin/socket.sk -w src -D test1/LRU/victims/ -u src/server.c,src/client.c

#read some files (read files are sent to the specified directory) and delete one file
echo -e "\n[LRU] Fourth client"
bin/client -p -t 200 -f bin/socket.sk -r files/3.txt,files/4.txt,src/server.c,src/client.c -d test1/LRU/read/ -c src/server.c

#send one file, locks it and deletes it
echo -e "\n[LRU] Fifth client"
bin/client -p -t 200 -f bin/socket.sk -W src/server.c -l src/server.c -c src/server.c

#tries to read all files from server (read files are sent to the specified directory)
echo -e "\n[LRU] Sixth client"
bin/client -p -t 200 -f bin/socket.sk -R n=0 -d test1/FIFO/read/

echo -e "\n[LRU] Client requests finished"

echo -e "\n[LRU] Sending SIGHUP to server"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "\n[LRU] LRU test 1 completed"

echo -e "\n== TEST 1 COMPLETED SUCCESSFULLY =="

rm -r files

exit 0
