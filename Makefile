CC=gcc

//CFLAGS=$(shell pkg-config --cflags gconf-2.0)
//LIBS=$(shell pkg-config --libs gconf-2.0)

CFLAGS+= -g -Wall -I.
#CFLAGS+= -O6 -I. -D_GNU_SOURCE -Wall -U_FORTIFY_SOURCE # -Wno-attributes

OBJS:= gpsdata.o webgpsd.o web.o kmlzipper.o #harley.o

all: webgpsd

webgpsd: $(OBJS)
	gcc -o $@ $^ $(LIBS)

web.o: web.c satstat.h dogmaphtml.h radfmt.h

radfmt.h: radfmt.html
	./html2head radfmt $<

satstat.h: satstat.html
	./html2head satstat $<

dogmaphtml.h: dogmap.html
	./html2head dogmaphtml $<

clean:
	rm -f webgpsd $(OBJS) kml2kmz dogmaphtml.h satstat.h radfmt.h *~
