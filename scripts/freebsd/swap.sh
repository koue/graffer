#!/bin/sh

if  [ -f "../compaths.conf" ]
then
        . ../compaths.conf
else
        echo 0
        exit
fi

if [ -z $1 ]
then
	$ECHO 0
	exit
fi


case "$1" in
	all)
		$SWAPINFO -m | $TAIL -1 | $AWK -F" " '{print $2}'
		;;

	used)
		$SWAPINFO -m | $TAIL -1 | $AWK -F" " '{print $3}'
		;;
	*)
		$ECHO 0
esac