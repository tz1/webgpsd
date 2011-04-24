CC=gcc

//CFLAGS=$(shell pkg-config --cflags gconf-2.0)
//LIBS=$(shell pkg-config --libs gconf-2.0)

#CFLAGS+= -g -Wall -I. -DHARLEY
CFLAGS+= -O6 -I. -D_GNU_SOURCE -Wall -DHARLEY #-U_FORTIFY_SOURCE # -Wno-attributes

OBJS:= gpsdata.o webgpsd.o web.o kmlzipper.o harley.o

all: webgpsd

webgpsd: $(OBJS)
	gcc -o $@ $^ $(LIBS)

web.o: web.c satstat.h dogmap.h radfmt.h hogstat.h

%.h : %.html
	./html2head $@

clean:
	rm -f webgpsd $(OBJS) kml2kmz dogmap.h satstat.h radfmt.h hogstat.h *~
