#!/bin/bash

cd /home/pi/projects/doorbell

mkfifo /tmp/mceiling
chmod 666 /tmp/mceiling
mplayer -idle -really-quiet -slave -ao alsa:device=dmix=2.0 -input file=/tmp/mceiling &

mkfifo /tmp/rnet
chmod 666 /tmp/rnet
./rnet /dev/ttyUSB0 -input /tmp/rnet &

# ./ssdp &
./gpio 4 &

#../airplay-audio-project/shairport/shairport -a 'Ceiling Speakers' -l /home/pi/shairport.txt &
