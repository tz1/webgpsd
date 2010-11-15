CC=gcc

//CFLAGS=$(shell pkg-config --cflags gconf-2.0)
//LIBS=$(shell pkg-config --libs gconf-2.0)

CFLAGS+= -g -Wall -I.
#CFLAGS+= -O6 -I. -U_FORTIFY_SOURCE -Wall # -Wno-attributes

OBJS:= gpsdata.o webgpsd.o web.o #harley.o

all: webgpsd kml2kmz

webgpsd: $(OBJS)
	gcc -o $@ $^ $(LIBS)

web.o: satstat.h dogmaphtml.h

satstat.h: satstat.html
	./html2head satstat $<

dogmaphtml.h: dogmap.html
	./html2head dogmaphtml $<

clean:
	rm -f webgpsd $(OBJS) kml2kmz dogmaphtml.h satstat.h *~
