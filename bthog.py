#!/usr/bin/python

import gobject
import sys
import dbus
import dbus.service
import dbus.mainloop.glib
import socket
import time

pincode = "0000"
status = 0;
btaddr = "none"

class Agent(dbus.service.Object):
	@dbus.service.method("org.bluez.Agent", in_signature="o", out_signature="s")
	def RequestPinCode(self, device):
		global btaddr
		print "SendPIN " + btaddr
		return pincode

def create_device_reply(device):
	global status
	status = 1
	mainloop.quit()

def create_device_error(error):
	print "Pair Fail"
	time.sleep(10)
	mainloop.quit()

if __name__ == '__main__':
        if len(sys.argv) < 2 :
		print "Usage: btgpsrc.py bt:ad:dr:es:ss:hx [pin-if-not-0000]"
		exit(1)

        out = socket.create_connection(('localhost',2947))
	mainloop = gobject.MainLoop()
        if len(sys.argv) > 2 :
                pincode = sys.argv[2]
	btaddr = sys.argv[1];
	dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
	bus = dbus.SystemBus()
	apath = "/test/hogagent"
	agent = Agent(bus, apath)
	manager = dbus.Interface(bus.get_object("org.bluez", "/"), "org.bluez.Manager")
	adapter = dbus.Interface(bus.get_object("org.bluez", manager.DefaultAdapter()), "org.bluez.Adapter")

	while(1):
		try:
			path = adapter.FindDevice(btaddr)
			adapter.RemoveDevice(path)
		except:
			pass

		status = 0;
		while ( 0 == status ):
			adapter.CreatePairedDevice(btaddr, apath, "DisplayYesNo",
					   reply_handler=create_device_reply,
					   error_handler=create_device_error)
			mainloop.run()

		path = adapter.FindDevice(btaddr)
		serial = dbus.Interface(bus.get_object("org.bluez", path), "org.bluez.Serial")
		node = 0;
		node = serial.Connect("spp")
		nodeid = node[11:]
		try:
			ser = open( node, "r")
		except:
			serial.Disconnect(node)
			adapter.RemoveDevice(path)
			time.sleep(3); #wait, then check for reconnect
			continue;

		ms = str(int(time.time()*1000%100000))
		out.send(":ANOBH"+nodeid+":"+ms+":"+btaddr+"\n")

		dat = ser.readline() #toss the first one
		while(1):
			dat = ser.readline()
			if( 0 == len(dat) ):
				break;
			ms = str(int(time.time()*1000%100000))
			ln = ":HOGB"+nodeid+":"+ms+":"+dat
			out.send(ln)
		#except: #bt conn broken
		print node +" Disconnect"
		if( node ):
			ser.close()
			serial.Disconnect(node)
		adapter.RemoveDevice(path)
		time.sleep(3); #wait, then check for reconnect
