#!/bin/sh

infile=$1

if [ X$infile = X ]
then
	echo Need json file as first argument
	exit 1
fi


if [ ! -f $infile ]
then
	echo File $infile cannot be opened
	exit 1
fi

sed 's/},/},\n/g' $infile | sed 's/.* \[{/ {/g' | awk '{ print $2 $4 $5 $7 $8 $10 $11 $12 $13}' | tr '"[]}' '    ' | sed 's/ , / /g' | sed 's/  *, *//g'
