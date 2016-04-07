#!/bin/bash

rnet /dev/ttyUSB0 savestate 1 rnet1.txt
rnet /dev/ttyUSB0 savestate 2 rnet2.txt
rnet /dev/ttyUSB0 savestate 3 rnet3.txt
rnet /dev/ttyUSB0 savestate 4 rnet4.txt
rnet /dev/ttyUSB0 savestate 5 rnet5.txt
rnet /dev/ttyUSB0 savestate 6 rnet6.txt

rnet /dev/ttyUSB0 loadstate doorbell.rnet

mplayer /home/root/projects/rnet/Doorbell.m4a -ao alsa:device=hw=1.0

rnet /dev/ttyUSB0 loadstate rnet1.txt
rnet /dev/ttyUSB0 loadstate rnet2.txt
rnet /dev/ttyUSB0 loadstate rnet3.txt
rnet /dev/ttyUSB0 loadstate rnet4.txt
rnet /dev/ttyUSB0 loadstate rnet5.txt
rnet /dev/ttyUSB0 loadstate rnet6.txt
