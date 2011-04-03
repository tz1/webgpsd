#!/usr/bin/python

import sys
import socket
import os
import time

out = socket.create_connection(('localhost',2947))

if len(sys.argv) > 1 :
	try:
		ser = open( sys.argv[1], "r")
	except:
		print "Usage: devgpsrc.py [/dev/if/not/stdin]"
		exit(1)
        nodeid = sys.argv[1][-4]
        ms = str(int(time.time()*1000%100000))
        out.send(":ANOB"+nodeid+":"+ms+":"+sys.argv[1]+"\n")
else:
        ser = sys.stdin
        nodeid = str(os.getpid())
        ms = str(int(time.time()*1000%100000))
        out.send(":ANOD"+nodeid+":"+ms+":(stdin)\n")

while(1):
        dat = ser.readline()
        ms = str(int(time.time()*1000%100000))
        ln = ":GPSB"+nodeid+":"+ms+":"+dat
        out.send(ln)

