#!/bin/sh

if  [ -f "`dirname $0`/../compaths.conf" ]
then
	. `dirname $0`/../compaths.conf
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
	user)
		$VMSTAT -c3 | $TAIL -1 | $AWK -F" " '{print $17}' 
		;;

	sys)
		$VMSTAT -c3 | $TAIL -1 | $AWK -F" " '{print $18}'
		;;

	*)
		$ECHO 0
esac
