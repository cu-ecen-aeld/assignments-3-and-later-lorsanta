#!/bin/bash

if [ $# -lt 2 ]; then echo "need two arguments"; exit 1; fi;
if ! [ -d $1 ]; then echo "first arg not a dir"; exit 1; fi;

m=0
for i in `grep $2 -hcr $1`; do m=$((m+$i)); done;
echo "The number of files are `find $1  -type f | wc -l` and the number of matching lines are $m"
