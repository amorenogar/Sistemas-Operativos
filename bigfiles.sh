#!/bin/bash

LIST=`du -a`
NLINES=2
echo $NLINES
if( $# == 1 );then
	echo solo hay un comando
fi
for x in $LIST;do
	if [ -f $x ]; then
		du -a $x
	fi
done | awk '{print $2 "\t" $1}' | sort -nr -k 2 | sed 2q
