CC=gcc

//CFLAGS=$(shell pkg-config --cflags gconf-2.0)
//LIBS=$(shell pkg-config --libs gconf-2.0)

#CFLAGS+= -g -Wall -I. -DHARLEY
CFLAGS+= -O6 -I. -D_GNU_SOURCE -Wall -DHARLEY #-U_FORTIFY_SOURCE # -Wno-attributes

OBJS:= gpsdata.o webgpsd.o web.o kmlzipper.o harley.o

all: webgpsd devgpsrc kmzmerge kml2kmz

webgpsd: $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

SERVHTML = dogmap.html satstat.html radfmt.html hogstat.html 
#HEADHTML = dogmap.h satstat.h radfmt.h hogstat.h 
#web.o: web.c $(HEADHTML)

install: all
	 mkdir -p /etc/webgpsd
	 cp $(SERVHTML) /etc/webgpsd
	 cp webgpsd devgpsrc /usr/bin

#%.h : %.html
#	./html2head $@

clean:
	rm -f webgpsd $(OBJS) kml2kmz $(HEADHTML) *~ hogdev j1850 devgpsrc kmzmerge
