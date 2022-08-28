#!/bin/bash

echo -e "\n== TEST 2 =="

echo -e "\nCreating test files..."

mkdir files1
touch files1/1.txt
touch files1/2.txt
touch files1/3.txt
touch files1/4.txt
touch files1/5.txt
touch files1/6.txt
touch files1/7.txt
touch files1/8.txt
touch files1/9.txt
touch files1/10.txt
head -c 300KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/1.txt
head -c 350KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/2.txt
head -c 400KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/3.txt
head -c 500KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/4.txt
head -c 600KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/5.txt
head -c 700KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/6.txt
head -c 800KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/7.txt
head -c 900KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/8.txt
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/9.txt
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/10.txt
cp -r files1 files2
cp -r files2 files3

echo -e "\nTest files created"

echo -e "\n[FIFO] Starting server"
bin/server config/test2/FIFOconfig.txt &


SERVER_PID=$!
export SERVER_PID
sleep 3s

echo -e "\n[FIFO] Starting clients"

#all clients send files (testing replacement algorithm, victims are sent to specified directory) 
echo -e "\n[FIFO] First client"
bin/client -p -t 100 -f bin/socket.sk -w files1 -D test2/FIFO/victims1/

echo -e "\n[FIFO] Second client"
bin/client -p -t 100 -f bin/socket.sk -w files2 -D test2/FIFO/victims2/

echo -e "\n[FIFO] Third client"
bin/client -p -t 100 -f bin/socket.sk -w files3 -D test2/FIFO/victims3/

echo -e "\n[FIFO] Client requests finished"

echo -e "\n[FIFO] Sending SIGHUP to server"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "\n[FIFO] FIFO test 2 completed"

echo -e "\n[LRU] Starting server"

bin/server config/test2/LRUconfig.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 3s

echo -e "\n[LRU] Starting clients"

#all clients send files (testing replacement algorithm, victims are sent to specified directory) 
echo -e "\n[LRU] First client"
bin/client -p -t 100 -f bin/socket.sk -w files1 -D test2/LRU/victims1/

echo -e "\n[LRU] Second client"
bin/client -p -t 100 -f bin/socket.sk -w files2 -D test2/LRU/victims2/

echo -e "\n[LRU] Third client"
bin/client -p -t 100 -f bin/socket.sk -w files3 -D test2/LRU/victims3/

echo -e "\n[LRU] Client requests finished"

echo -e "\n[LRU] Sending SIGHUP to server"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "\n[LRU] LRU test 2 completed"

echo -e "\n== TEST 2 COMPLETED SUCCESSFULLY =="

rm -r files1
rm -r files2
rm -r files3

exit 0
