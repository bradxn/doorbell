#!/bin/bash

cd /home/pi/projects/doorbell

mkfifo /tmp/mceiling
chmod 666 /tmp/mceiling
mplayer -idle -really-quiet -slave -input file=/tmp/mceiling &

./ssdp &
./gpio 18 &

../airplay-audio-project/shairport/shairport -a 'Ceiling Speakers' &
