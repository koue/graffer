#!/bin/sh

if  [ -f "`dirname $0`/../compaths.conf" ]
then
        . `dirname $0`/../compaths.conf
else
        echo 0
        exit
fi

if [ -z $1 ] || [ -z $2 ]
then
	$ECHO 0
	exit
fi

iface=$1

case "$2" in
	in)
		$NETSTAT -bI $iface | $GREP "<Link#.*>" | $AWK -F" " '{print $8}'
		;;

	out)
		$NETSTAT -bI $iface | $GREP "<Link#.*>" | $AWK -F" " '{print $11}'
		;;

	*)
		$ECHO 0
esac
