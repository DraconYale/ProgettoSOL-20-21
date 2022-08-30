#!/bin/bash

echo -e "\n== TEST 3 =="

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
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/1.txt
head -c 5MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/2.txt
head -c 10MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/3.txt
head -c 15MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/4.txt
head -c 20MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/5.txt
head -c 25MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/6.txt
head -c 25MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/7.txt
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/8.txt
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/9.txt
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold  > files1/10.txt
cp -r files1 files2
cp -r files1 files3
cp -r files1 files4
cp -r files1 files5
cp -r files1 files6
cp -r files1 files7
cp -r files1 files8
cp -r files1 files9
cp -r files1 files10

echo -e "\nTest files created"

echo -e "\n[FIFO] Starting server"
bin/server config/test3/FIFOconfig.txt &

SERVER_PID=$!
export SERVER_PID
sleep 5s

bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &
echo -e "\n[FIFO] SIGINT to server in 30 seconds"

echo -e "\n[FIFO] Starting clients"

#starts 10 clients
clients_pids=()
for i in {1..10}; do
	bash -c 'script/test3Clients.sh' &
	clients_pids+=($!)
	sleep 0.1
done

sleep 30s

echo -e "\n[FIFO] Killing clients"

#kills clients
for i in "${clients_pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null #stderror redirected to /dev/null
done

wait $SERVER_PID
killall -q bin/client #if some clients are still running
echo -e "\n[FIFO] FIFO test 3 completed"


echo -e "\n[LRU] Starting server"
bin/server config/test3/LRUconfig.txt &

SERVER_PID=$!
export SERVER_PID
sleep 5s

bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &
echo -e "\n[LRU] SIGINT to server in 30 seconds"

#starts 10 clients
clients_pids=()
for i in {1..10}; do
	bash -c 'script/test3Clients.sh' &
	clients_pids+=($!)
	sleep 0.1
done

sleep 30s

#kills clients
for i in "${clients_pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null #stderror redirected to /dev/null
done

wait $SERVER_PID
killall -q bin/client #if some clients are still running
echo -e "\n[LRU] LRU test 3 completed"

echo -e "\n== TEST 3 COMPLETED SUCCESSFULLY =="

rm -r files1
rm -r files2
rm -r files3
rm -r files4
rm -r files5
rm -r files6
rm -r files7
rm -r files8
rm -r files9
rm -r files10

exit 0

