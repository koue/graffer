#!/bin/sh
PATHS=`dirname $0`"/compaths.conf"

if [ -f $PATHS ]
then
	echo "Command paths are configured already."
	echo "Remove $PATHS and run $0 again."
	exit
fi

for COMMAND in \
		awk \
		cut \
		du \
		df \
		echo \
		grep \
		head \
		iostat \
		netstat \
		pwd \
		smartctl \
		systat \
		swapinfo \
		tail \
		tr \
		vmstat \
		wc
do
	COMMAND_PATH=`which $COMMAND`
	if [ $? -ne 0 ]
	then
		echo "# [ $COMMAND ] command has not been found" >> $PATHS
	else
		OUTPUT_1=`echo $COMMAND | tr [:lower:] [:upper:]`
		echo $OUTPUT_1=$COMMAND_PATH >> $PATHS
	fi
done
