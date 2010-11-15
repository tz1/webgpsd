#include "webgpsd.h"

// web pages from server
#include "dogmaphtml.h"
#include "satstat.h"

//kml feed

static char gpskml[] = "<Document>"
  "<flyToView>1</flyToView>"
  "<name>%s %d.%03d</name>"
  "<LookAt>"
  "<longitude>%d.%06d</longitude>\n"
  "<latitude>%d.%06d</latitude>\n"
  "<range>%d</range>"
  "<tilt>%d</tilt>"
  "<heading>%d.%03d</heading>"
  "</LookAt>"
  "<Placemark>"
  "<Style><IconStyle><Icon><href>"
  "root://icons/high_res_places.png"
  "</href></Icon></IconStyle></Style>"
  "<Point><coordinates>" "%d.%06d,%d.%06d" "</coordinates></Point>" "</Placemark>" "</Document>\n";

void dokml(char *c)
{
    char *d;
    int range = 1500, tilt = 45;
    d = strstr(c, "kml");
    d--;
    d--;
    while (*d && (*d >= '0' && *d <= '9'))
        d--;
    range = atoi(&d[1]);
    d--;
    while (*d && (*d >= '0' && *d <= '9'))
        d--;

    tilt = atoi(&d[1]);

    sprintf(xbuf, gpskml, rtname, gpst[bestgps].gspd / 1000, gpst[bestgps].gspd % 1000,
            gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000),
            gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000), range, tilt,
            gpst[bestgps].gtrk / 1000, gpst[bestgps].gtrk % 1000,
            gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000),
            gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000));
}

//moving google map

static char xmldata[] = "<xml><markers><marker lat=\"%d.%06d\" lng=\"%d.%06d\"/></markers></xml>";

static void doxml()
{
    sprintf(xbuf, xmldata, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
            gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000));
}

static char jsondata[] = "{lat:%d.%06d,lon:%d.%06d,alt:%d.%03d,pdop:%d.%02d,hdop:%d.%02d,vdop:%d.%02d,track:%d.%03d,speed:%d.%03d,mode:%d,ns:%d,\r\nu:[";

static void dojson()
{
    sprintf(xbuf, jsondata, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
            gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000),
            gpst[bestgps].alt / 1000, abs(gpst[bestgps].alt % 1000),
            gpst[bestgps].pdop / 1000, gpst[bestgps].pdop % 1000 / 10,
            gpst[bestgps].hdop / 1000, gpst[bestgps].hdop % 1000 / 10,
            gpst[bestgps].vdop / 1000, gpst[bestgps].vdop % 1000 / 10, 
            gpst[bestgps].gtrk / 1000, gpst[bestgps].gtrk % 1000,
            gpst[bestgps].gspd / 1000, gpst[bestgps].gspd % 1000, gpst[bestgps].fix, gpst[bestgps].nsats);
            int n;
            char cbuf[64];
            for (n = 0; n < gpst[bestgps].nsats; n++) {
                if(n)
                    strcat(xbuf, ",");
                sprintf(cbuf, "%d", gpst[bestgps].sats[n].num < 0);
                strcat(xbuf, cbuf);
            }
            strcat(xbuf, "],\r\nn:[");
            for (n = 0; n < gpst[bestgps].nsats; n++) {
                if(n)
                    strcat(xbuf, ",");
                sprintf(cbuf, "%d",  abs(gpst[bestgps].sats[n].num));
                strcat(xbuf, cbuf);
            }
            strcat(xbuf, "],\r\nel:[");
            for (n = 0; n < gpst[bestgps].nsats; n++) {
                if(n)
                    strcat(xbuf, ",");
                sprintf(cbuf, "%d", gpst[bestgps].sats[n].el);
                strcat(xbuf, cbuf);
            }
            strcat(xbuf, "],\r\naz:[");
            for (n = 0; n < gpst[bestgps].nsats; n++) {
                if(n)
                    strcat(xbuf, ",");
                sprintf(cbuf, "%d", gpst[bestgps].sats[n].az);
                strcat(xbuf, cbuf);
            }
            strcat(xbuf, "],\r\nsn:[");
            for (n = 0; n < gpst[bestgps].nsats; n++) {
                if(n)
                    strcat(xbuf, ",");
                sprintf(cbuf, "%d", gpst[bestgps].sats[n].sn);
                strcat(xbuf, cbuf);
            }
            strcat(xbuf,"]}\r\n");

}

static void dogmap()
{
    strcpy(xbuf, dogmaphtml);
}

static char radfmt[] = 
"<html><head><title>WeatherRadar</title><meta http-equiv=\"refresh\" content=\"60\"></head><body><table>"
"<tr><td height=360>"

"<table border=1><tr><td><a href=/><h1>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"<br>BACK<br>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"</a>"
"<tr><td><a href=/radar%d.html><h1>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"<br>Zoom +<br>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"</a>"
"<tr><td><a href=/radar%d.html><h1>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"<br>Zoom -<br>"
"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"</a>"
"</table>"

"<td width=360 height=360>"
"<IFRAME align=right width=100%% height=100%% src="
"\"http://radblast-mi.wunderground.com/cgi-bin/radar/WUNIDS_composite?centerlat=%d.%06d&centerlon=%d.%06d&radius=%d"
"&type=N0R&frame=0&num=5&delay=10&width=340&height=340&newmaps=1&r=1159834180&showstorms=0&theme=WUNIDS_severe&rainsnow=1\""
"></IFRAME>"
"</table></body></html>\n";

static void dorad(int size)
{
    sprintf(xbuf, radfmt, size/2, size*2, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000), gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000), size );
}

//standard gps view

static char gpspage1[] = "<HEAD><TITLE>%s</TITLE><meta http-equiv=\"refresh\" content=\"5\"></HEAD><BODY>\n"
    "<table><tr><td>"
    "\n<table border=1><tr><td><a href=/dogmap.html><h1>"
    "&nbsp;&nbsp;MAP&nbsp;&nbsp;<br>"
    "</a></tr>"
    "<tr><td><a href=/sats.html><h1>"
    "&nbsp;&nbsp;SatStat&nbsp;&nbsp;<br>"
    "</a></tr>"
    "<tr><td><a href=/radar20.html><h1>"
    "&nbsp;&nbsp;RADAR&nbsp;&nbsp;<br>"
    "</a></tr></table>\n";

static char gpspage2[] = 
  "<td>\n<table border=%d><tr><td>"
  "%02d-%02d-%02d %02d:%02d:%02d<tr><td>"
  "lat=%d.%06d<tr><td>lon=%d.%06d<tr><td>"
  "%dD fix<tr><td>%s lock<tr><td>"
  "alt=%d.%03d<tr><td>"
  "spd=%d.%03d<tr><td>"
  "trk=%d.%03d<tr><td>" "PDoP=%d.%02d<tr><td>" "HDoP=%d.%02d<tr><td>" "VDoP=%d.%02d</table>\n"
  "<td>\n<table border=%d><tr><th>SN<th>EL<th>AZM<th>SG<th>U</tr>";

static char gpspage9[] = "</table>\n</BODY></HTML>\r\n\r\n";

static void doweb()
{
    char *c;
    int n, i;
    c = strchr(xbuf, '&');
    if (!c)
        c = "";
    sprintf(xbuf, gpspage1, rtname);
    for( i = 0 ; i < MAXSRC; i++ ) {
        if( gpst[i].gpsfd < -1 )
            break;
        if( gpst[i].gpsfd < 0 )
            continue;
        sprintf(&xbuf[strlen(xbuf)], gpspage2, i == bestgps ? 3 : 1, gpst[i].yr, gpst[i].mo, gpst[i].dy,
            gpst[i].hr, gpst[i].mn, gpst[i].sc,
            gpst[i].llat / 1000000, abs(gpst[i].llat % 1000000), gpst[i].llon / 1000000,
            abs(gpst[i].llon % 1000000), gpst[i].fix, gpst[i].lock ? (gpst[i].lock == 1 ? "GPS" : "DGPS") : "no",
            gpst[i].alt / 1000, abs(gpst[i].alt % 1000)
            , gpst[i].gspd / 1000, gpst[i].gspd % 1000, gpst[i].gtrk / 1000, gpst[i].gtrk % 1000,
            gpst[i].pdop / 1000, gpst[i].pdop % 1000 / 10, gpst[i].hdop / 1000, gpst[i].hdop % 1000 / 10,
                gpst[i].vdop / 1000, gpst[i].vdop % 1000 / 10, i == bestgps ? 3 : 1);
        for (n = 0; n < gpsat[i].nsats; n++) {
            int m;
            sprintf(&xbuf[strlen(xbuf)], "<tr><td>%02d<td>%02d<td>%03d<td>%02d<td>", gpsat[i].view[n],
                    gpsat[i].el[gpsat[i].view[n]], gpsat[i].az[gpsat[i].view[n]],
                    gpsat[i].sn[gpsat[i].view[n]]);
            for (m = 0; m < 12; m++)
                if (gpsat[i].sats[m] == gpsat[i].view[n])
                    break;
            sprintf(&xbuf[strlen(xbuf)], "%s</tr>", m == 12 ? " " : "*");
        }
        sprintf(&xbuf[strlen(xbuf)], "</table>\n</td>\n");
    }
    strcpy(&xbuf[strlen(xbuf)], gpspage9);
}

static void dosats() {
    strcpy( xbuf, satstat );
}

void dowebget() {
    char *c;
    if (strstr(xbuf, "kml"))
        dokml(xbuf);
    else if (strstr(xbuf, "json"))
        dojson(xbuf);
    else if (strstr(xbuf, "gpsdata.xml"))
        doxml();
    else if (strstr(xbuf, "dogmap.html"))
        dogmap();
    else if ((c = strstr(xbuf, "radar"))) {
        c += 5;
        int i = atoi(c);
        if( !i )
            i = 20;
        dorad(i);
    }
    else if ((c = strstr(xbuf, "sats"))) {
        dosats();
    }
    else
        doweb();
}
