#!/bin/bash
echo "Content-Type: text/plain"
echo ""
saveIFS=$IFS
IFS='=&'
parm=($QUERY_STRING)
IFS=$saveIFS
declare -A array
for ((i=0; i<${#parm[@]}; i+=2))
do
    array[${parm[i]}]=${parm[i+1]}
    echo ${parm[i+1]}
done
echo "OK"
