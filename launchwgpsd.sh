#!/bin/sh
./webgpsd -i 5 -l $HOME -p 2947 &
./btgpsrc.py 00:10:08:24:02:23 &
./bthog.py 00:10:08:24:02:34 &
