#!/usr/bin/python

import sys
import socket
import os
import time
import zipfile

out = socket.create_connection(('localhost',2947))

for fl in sys.argv:
        try:
                zf = zipfile.ZipFile(fl,"r")
                for zifn in zf.namelist():
                        zdat = zf.read(zifn)
                        for ln in zdat.splitlines():
                                ix = ln.find("<!-- :")
                                if( ix == 0 ):
                                        out.send(ln[5:-4]+"\r\n")
                                        time.sleep(0.01)
        except:
                ser = open(fl, "r")
                while(1):
                        ln = ser.readline()
                        if( 0 == len(ln) ):
                                break;
                        ix = ln.find("<!-- :")
                        if( ix == 0 ):
                                out.send(ln[5:-4]+"\r\n")
                                time.sleep(0.01)
#                                print ln[5:-4]
