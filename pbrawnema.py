#!/usr/bin/python

import sys
import socket
import os
import time
import zipfile

out = socket.create_connection(('localhost',2947))

for fl in sys.argv:
	i = 0;
	ser = open(fl, "r")
	while(1):
		time.sleep(0.005)
		dat = ser.readline()
		if( 0 == len(dat) ):
			break;
		ms = str(int(time.time()*1000%100000))
		ln = ":GPSNEMAPB:"+ms+":"+dat+"\r\n"
		out.send(ln)
