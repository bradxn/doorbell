#!/bin/bash
#QUERY_STRING="loadlist http://somafm.com/startstream=groovesalad.pls"

echo "Content-Type: text/plain"
echo ""
echo "${QUERY_STRING/\+/ }" > /tmp/mceiling
echo "${QUERY_STRING/\+/ }" > /home/pi/foobar.txt
echo "${QUERY_STRING/\+/ }"
echo "OK"
