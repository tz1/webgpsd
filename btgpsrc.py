#!/usr/bin/python

import gobject
import sys
import dbus
import dbus.service
import dbus.mainloop.glib
import socket
import time

pincode = "0000"
class Agent(dbus.service.Object):
	@dbus.service.method("org.bluez.Agent", in_signature="o", out_signature="s")
	def RequestPinCode(self, device):
		return pincode

def create_device_reply(device):
	mainloop.quit()

def create_device_error(error):
	print "PIN Failed"
        exit(-1)

if __name__ == '__main__':
	dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
	bus = dbus.SystemBus()
	manager = dbus.Interface(bus.get_object("org.bluez", "/"), "org.bluez.Manager")
	adapter = dbus.Interface(bus.get_object("org.bluez", manager.DefaultAdapter()), "org.bluez.Adapter")

        try:
                device = adapter.FindDevice(sys.argv[1])
                adapter.RemoveDevice(device)
        except:
                pass

	path = "/test/agent"
	agent = Agent(bus, path)
	mainloop = gobject.MainLoop()

        if len(sys.argv) > 2 :
                pincode = sys.argv[2]
	adapter.CreatePairedDevice(sys.argv[1], path, "DisplayYesNo",
					reply_handler=create_device_reply,
					error_handler=create_device_error)
	mainloop.run()

        bus = dbus.SystemBus()
        manager = dbus.Interface(bus.get_object("org.bluez", "/"), "org.bluez.Manager")
        apath = manager.DefaultAdapter()
        adapter = dbus.Interface(bus.get_object("org.bluez", apath), "org.bluez.Adapter")
        path = adapter.FindDevice(sys.argv[1])
        serial = dbus.Interface(bus.get_object("org.bluez", path), "org.bluez.Serial")
        node = serial.Connect("spp")
        nodeid = node[11:]
        ser = open( node, "r")
        out = socket.create_connection(('localhost',2947))
        ms = str(int(time.time()*1000%100000))
        out.send(":ANOB"+nodeid+":"+ms+":"+sys.argv[1]+"\n")
        try:
                dat = ser.readline() #toss the first one
                while(1):
                        dat = ser.readline()
                        ms = str(int(time.time()*1000%100000))
                        ln = ":GPSB"+nodeid+":"+ms+":"+dat
                        out.send(ln)
        except:
                serial.Disconnect(node)

