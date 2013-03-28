#!/bin/sh

if  [ -f "../compaths.conf" ]
then
        . ../compaths.conf
else
        echo 0
        exit
fi

if [ -z "$1" ]
then
	$ECHO 0
	exit
fi

case "$2" in
	temp)
		$SMARTCTL -a $1 | $GREP ^194 | $AWK -F" " '{print $10}'
		;;

	*)
		$ECHO 0
esac
