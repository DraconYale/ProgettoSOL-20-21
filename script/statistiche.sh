#!/bin/bash

if [ $# -eq 0 ]; then
	echo "Please specify a log file"
	else
	    LOG_FILE=$1
fi

echo -e "\n==Read operations=="

N_READ=$(grep "readFile" -c $LOG_FILE)

N_READN=$(grep "readNFile" -c $LOG_FILE)

READBYTES=$(grep "Read size:" $LOG_FILE | grep -oE '[^ ]+$' | sed 's/[^0-9]*//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum;})

N_READS=$((N_READ+N_READN))

echo -e "\nNumber of read operations: ${N_READS}"

if [ ${N_READS} -gt 0 ]; then
	AV_READBYTES=$(echo "scale=3; ${READBYTES} / ${N_READS}" | bc -l)
	echo -e "Avarage read size: ${AV_READBYTES} bytes."
fi

echo -e "\n==Write operations=="

N_WRITE=$(grep "writeFile" -c $LOG_FILE)

N_APPEND=$(grep "appendToFile" -c $LOG_FILE)

WRITEBYTES=$(grep "Write size:" $LOG_FILE | grep -oE '[^ ]+$' | sed 's/[^0-9]*//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum;})

N_WRITES=$((N_WRITE+N_APPEND))

echo -e "\nNumber of write operations: ${N_WRITES}"
if [ ${N_READS} -gt 0 ]; then
	AV_WROTEBYTES=$(echo "scale=3; ${WRITEBYTES} / ${N_WRITES}" | bc -l)
	echo -e "Avarage read size: ${AV_WROTEBYTES} bytes."
fi

echo -e "\n==Open/Lock/Unlock/Close operations=="

N_LOCK=$(grep " lockFile" -c $LOG_FILE)
N_OPEN_LOCK_CREATE=$(grep -E "openFile .* 3" -c $LOG_FILE)
N_OPEN_LOCK=$(grep -E "openFile .* 2" -c $LOG_FILE)
N_OPEN_LOCKS=$((N_OPEN_LOCK_CREATE+N_OPEN_LOCK))
N_UNLOCK=$(grep "unlockFile" -c $LOG_FILE)
N_CLOSE=$(grep "closeFile" -c $LOG_FILE)

echo -e "\nNumber of lock operations: ${N_LOCK}"
echo -e "Number of open-lock operations: ${N_OPEN_LOCKS}"
echo -e "Number of unlock operations: ${N_UNLOCK}"
echo -e "Number of close operations: ${N_CLOSE}"

echo -e "\n==Server infos=="

MAXBYTES=$(grep "Max megabytes stored:" $LOG_FILE | sed 's/[^0-9]*//g')
MAXMEGAB=$(echo "scale=5; ${MAXBYTES} * 0.000001" | bc -l)
MAXFILES=$(grep "Max number of files stored:" $LOG_FILE | sed 's/[^0-9]*//g')
N_VICTIMS=$(grep "Replacement algorithm" $LOG_FILE | sed 's/[^0-9]*//g')

echo -e "\nMax megabytes stored: ${MAXMEGAB} MB"
echo -e "Max number of files: ${MAXFILES}"
echo -e "Replacement algorithm executed ${N_VICTIMS} times"

echo -e "\n"

T_REQUESTS=$(grep "Thread" $LOG_FILE | sed -s 's/.*Thread \([0-9]\+\):.*/\1/' | sort | uniq -c)

while IFS= read -r line; do
	array=($line)
	echo -e "Thread ID ${array[1]} served ${array[0]} requests"
done <<< "$T_REQUESTS"

CLIENTS=$(grep "client" $LOG_FILE)

MAX=0
TMP=0
CONN='connection'
DIS='disconnected'
while IFS= read -r line; do
	if [[ "$line" == *"$CONN"* ]]; then
		((TMP+=1))
		if [[ ${TMP} -gt ${MAX} ]]; then
			((MAX=TMP))
		fi
	fi
	if [[ "$line" == *"$DIS"* ]]; then
		((TMP-=1))
	fi
done <<< "$CLIENTS"

echo -e "\nMax simultaneus connections ${MAX}"

echo -e "\n"
