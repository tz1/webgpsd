#!/bin/sh
echo launching webgpsd and helpers
killall webgpsd
rm /var/run/webgpsd.pid
(while true;do
        if [ ! -e /var/run/webgpsd.pid -a `cat /sys/class/power_supply/ac/online` -eq "1" ] ; then 
                webgpsd -r -l /mnt/storage/ -k /var/run/webgpsd.pid -b /psp/gpsd.dat
                rm /var/run/webgpsd.pid
        fi
        sleep 5
done) &
sleep 1 #let webgpsd startup

(while true; do devgpsrc /dev/ttyUSB0; sleep 5; done)  &
(while true; do hogdev; sleep 5; done) &

mDNSPublish `hostname` _gpsd._tcp 2947 &

( while true; do
if [ `cat /sys/class/power_supply/ac/online` -eq "0" ] ; then 
        if [ `cat /sys/class/power_supply/battery/capacity` -lt "55" ]; then
                killall webgpsd
                sleep 5
        fi
        if [ `cat /sys/class/power_supply/battery/capacity` -lt "40" ]; then
                poweroff
        fi
fi
sleep 5
done ) &
echo webgpsd started
