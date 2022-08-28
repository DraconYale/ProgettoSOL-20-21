#!/bin/bash
while true
do
	#shuf -i 1-10 -n 1 generates a random number in range 1-10 and prints it at most -n 1 lines
	bin/client -f bin/socket.sk -w files$(shuf -i 1-10 -n 1)
done
