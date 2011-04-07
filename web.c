#include "webgpsd.h"

// web pages from HTML templat'es
#include "dogmap.h"
#include "satstat.h"
#include "radfmt.h"

// kml network link feed for google earth
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

// moving google map v2 API XML, see GDownloadUrl
static char xmldata[] = "<xml><markers><marker lat=\"%d.%06d\" lng=\"%d.%06d\"/></markers></xml>";
static void doxml()
{
    sprintf(xbuf, xmldata, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
      gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000));
}

// JSON for web - generally NOT gpsd-ng (that will go into the mainline)
static char jsondata[] =
  "{lat:%d.%06d,lon:%d.%06d,alt:%d.%03d,pdop:%d.%02d,hdop:%d.%02d,vdop:%d.%02d,track:%d.%03d,speed:%d.%03d,mode:%d,ns:%d,\r\nu:[";
static void dojson()
{
    int spd = gpst[bestgps].gspd;       // * 447 / 1000; // mph to m/s
    sprintf(xbuf, jsondata, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
      gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000),
      gpst[bestgps].alt / 1000, abs(gpst[bestgps].alt % 1000),
      gpst[bestgps].pdop / 1000, gpst[bestgps].pdop % 1000 / 10,
      gpst[bestgps].hdop / 1000, gpst[bestgps].hdop % 1000 / 10,
      gpst[bestgps].vdop / 1000, gpst[bestgps].vdop % 1000 / 10,
      gpst[bestgps].gtrk / 1000, gpst[bestgps].gtrk % 1000,
      spd / 1000, spd % 1000, gpst[bestgps].fix, gpst[bestgps].pnsats + gpst[bestgps].lnsats);
    int n;
    char cbuf[128];
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].psats[n].num < 0);
        strcat(xbuf, cbuf);
    }
    for (n = 0; n < gpst[bestgps].lnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].lsats[n].num < 0);
        strcat(xbuf, cbuf);
    }
    xbuf[strlen(xbuf) - 1] = 0; // zap trailing comma
    strcat(xbuf, "],\r\nn:[");
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        sprintf(cbuf, "%d,", abs(gpst[bestgps].psats[n].num));
        strcat(xbuf, cbuf);
    }
    for (n = 0; n < gpst[bestgps].lnsats; n++) {
        sprintf(cbuf, "%d,", abs(gpst[bestgps].lsats[n].num));
        strcat(xbuf, cbuf);
    }
    xbuf[strlen(xbuf) - 1] = 0; // zap trailing comma
    strcat(xbuf, "],\r\nel:[");
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].psats[n].el);
        strcat(xbuf, cbuf);
    }
    for (n = 0; n < gpst[bestgps].lnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].lsats[n].el);
        strcat(xbuf, cbuf);
    }
    xbuf[strlen(xbuf) - 1] = 0; // zap trailing comma
    strcat(xbuf, "],\r\naz:[");
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].psats[n].az);
        strcat(xbuf, cbuf);
    }
    for (n = 0; n < gpst[bestgps].lnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].lsats[n].az);
        strcat(xbuf, cbuf);
    }
    xbuf[strlen(xbuf) - 1] = 0; // zap trailing comma
    strcat(xbuf, "],\r\nsn:[");
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].psats[n].sn);
        strcat(xbuf, cbuf);
    }
    for (n = 0; n < gpst[bestgps].lnsats; n++) {
        sprintf(cbuf, "%d,", gpst[bestgps].lsats[n].sn);
        strcat(xbuf, cbuf);
    }
    xbuf[strlen(xbuf) - 1] = 0; // zap trailing comma
    strcat(xbuf, "]}\r\n");

}

// JSON gpsd-ng watch format
char ngsjson0[] = "{\"class\":\"SKY\",\"tag\":\"GSV\",\"vdop\":%d.%02d,\"hdop\":%d.%02d,\"pdop\":%d.%02d,\"satellites\":[";
char ngsjson1[] = "{\"PRN\":%d,\"el\":%d,\"az\":%d,\"ss\":%d,\"used\":%s},";
// note that it returns alt even in 2d and the others even if no fix.
char ngfjson1[] = "{\"class\":\"TPV\",\"tag\":\"RMC\",\"time\":\"%4d-%02d-%02dT%02d:%02d:%02d.%02dZ\",\"mode\":%d}\r\n";
char ngfjson2[] =
  "{\"class\":\"TPV\",\"tag\":\"RMC\",\"time\":\"20%02d-%02d-%02dT%02d:%02d:%02d.%02dZ\","
  "\"lat\":%d.%6d,\"lon\":%d.%6d,\"track\":%d.%03d,\"speed\":%d.%03d,\"mode\":%d}\r\n";
char ngfjson3[] =
  "{\"class\":\"TPV\",\"tag\":\"RMC\",\"time\":\"20%02d-%02d-%02dT%02d:%02d:%02d.%02dZ\","
  "\"lat\":%d.%6d,\"lon\":%d.%6d,\"alt\":%d.%03d,\"track\":%d.%03d,\"speed\":%d.%03d,\"mode\":%d}\r\n";
void dongjson()
{
    char *c;
    // fix
    switch (gpst[bestgps].fix) {
    case 2:
        sprintf(xbuf, ngfjson2,
          gpst[bestgps].yr, gpst[bestgps].mo, gpst[bestgps].dy, gpst[bestgps].hr, gpst[bestgps].mn, gpst[bestgps].sc,
          gpst[bestgps].scth / 10, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000), gpst[bestgps].llon / 1000000,
          abs(gpst[bestgps].llon % 1000000), gpst[bestgps].gtrk / 1000,
          gpst[bestgps].gtrk % 1000, gpst[bestgps].gspd / 1000, gpst[bestgps].gspd % 1000, gpst[bestgps].fix);
        break;
    case 3:
        sprintf(xbuf, ngfjson3,
          gpst[bestgps].yr, gpst[bestgps].mo, gpst[bestgps].dy, gpst[bestgps].hr, gpst[bestgps].mn, gpst[bestgps].sc,
          gpst[bestgps].scth / 10, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000), gpst[bestgps].llon / 1000000,
          abs(gpst[bestgps].llon % 1000000), gpst[bestgps].alt / 1000, abs(gpst[bestgps].alt % 1000), gpst[bestgps].gtrk / 1000,
          gpst[bestgps].gtrk % 1000, gpst[bestgps].gspd / 1000, gpst[bestgps].gspd % 1000, 3);
        break;
    default:
        {
            struct timeval tv;
            struct tm *tmpt;
            gettimeofday(&tv, NULL);
            tmpt = gmtime(&tv.tv_sec);
            sprintf(xbuf, ngfjson1,
              tmpt->tm_year, 1 + tmpt->tm_mon, tmpt->tm_mday, tmpt->tm_hour, tmpt->tm_min, tmpt->tm_sec, tv.tv_usec / 10000,
              gpst[bestgps].fix);
        }
        break;
    }
    // satelliter status
    c = xbuf + strlen(xbuf);
    sprintf(c, ngsjson0,
      gpst[bestgps].vdop / 1000, gpst[bestgps].vdop % 1000 / 10,
      gpst[bestgps].hdop / 1000, gpst[bestgps].hdop % 1000 / 10, gpst[bestgps].pdop / 1000, gpst[bestgps].pdop % 1000 / 10);
    int n;
    for (n = 0; n < gpst[bestgps].pnsats; n++) {
        c += strlen(c);
        sprintf(c, ngsjson1,
          abs(gpst[bestgps].psats[n].num),
          gpst[bestgps].psats[n].el, gpst[bestgps].psats[n].az, gpst[bestgps].psats[n].sn,
          gpst[bestgps].psats[n].num < 0 ? "true" : "false");
    }
    c += strlen(c);
    c--;
    *c = 0;                     // trailing comma
    strcat(c, "]}\r\n");

}

// Google Map - self contained, uses web json
static void dogmapx()
{
    strcpy(xbuf, dogmap);
}

// Wunderground.com severe weather radar, mostly self-contained with refreshing
static void dorad(int size, int picwidth, int picheight)
{
    sprintf(xbuf, radfmt, size / 2, size * 2, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
      gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000), size, picwidth, picheight);
}

// Standard gps view - menu plus gps source status
static char gpspage1[] =        // menu
  "<HEAD><TITLE>%s</TITLE><meta http-equiv=\"refresh\" content=\"5\">"
  "<meta name = \"viewport\" content = \"width = 480\">"
  "<style type=\"text/css\">table a {display: block; width: 100%; height: 100%}</style>"
  "</HEAD><BODY>\n"
  "<table><tr><td>"
  "\n<table border=1>"
  "<tr><td><a href=/dogmap.html><h1><br>MAP<br><br></a>"
  "<tr><td><a href=/sats.html><h1><br>SatStat<br><br></a>" "<tr><td><a href=/radar20.html><h1><br>RADAR<br><br></a>" "</table>\n";

static char gpspage2[] =        // source status
  "<td>\n<table border=%d><tr><td>"
  "%02d-%02d-%02d %02d:%02d:%02d<tr><td>"
  "lat=%d.%06d<tr><td>lon=%d.%06d<tr><td>"
  "%dD fix<tr><td>%s lock<tr><td>"
  "alt=%d.%03d<tr><td>"
  "spd=%d.%03d<tr><td>"
  "trk=%d.%03d<tr><td>" "PDoP=%d.%02d<tr><td>" "HDoP=%d.%02d<tr><td>" "VDoP=%d.%02d</table>\n"
  "<td>\n<table border=%d><tr><th>PRN<th>Elev<th>Azim<th>Sgnl<th>Usd</tr>";

static char gpspage9[] =        // closing
  "</table>\n</BODY></HTML>";

static void doweb()
{
    char *c;
    int i,n;
    c = strchr(xbuf, '&');
    if (!c)
        c = "";
    sprintf(xbuf, gpspage1, rtname);
    for (i = 0; i < MAXSRC; i++) {
        if (gpst[i].gpsfd < -1)
            break;
        if (gpst[i].gpsfd < 0)
            continue;
        sprintf(&xbuf[strlen(xbuf)], gpspage2, i == bestgps ? 3 : 1, gpst[i].yr, gpst[i].mo, gpst[i].dy,
          gpst[i].hr, gpst[i].mn, gpst[i].sc,
          gpst[i].llat / 1000000, abs(gpst[i].llat % 1000000), gpst[i].llon / 1000000,
          abs(gpst[i].llon % 1000000), gpst[i].fix, gpst[i].lock ? (gpst[i].lock == 1 ? "GPS" : "DGPS") : "no",
          gpst[i].alt / 1000, abs(gpst[i].alt % 1000)
          , gpst[i].gspd / 1000, gpst[i].gspd % 1000, gpst[i].gtrk / 1000, gpst[i].gtrk % 1000,
          gpst[i].pdop / 1000, gpst[i].pdop % 1000 / 10, gpst[i].hdop / 1000, gpst[i].hdop % 1000 / 10,
          gpst[i].vdop / 1000, gpst[i].vdop % 1000 / 10, i == bestgps ? 3 : 1);
	// sats
	for (n = 0; n < gpst[i].pnsats; n++) 
	  sprintf(&xbuf[strlen(xbuf)], "<tr><td>%d<td>%d<td>%d<td>%d<td>%c</tr>\n",
		  abs(gpst[i].psats[n].num),
		  gpst[i].psats[n].el,
		  gpst[i].psats[n].az,
		  gpst[i].psats[n].sn,
		  gpst[i].psats[n].num < 0 ?'*':' ');
	for (n = 0; n < gpst[i].lnsats; n++) 
	  sprintf(&xbuf[strlen(xbuf)], "<tr><td>%d<td>%d<td>%d<td>%d<td>%c</tr>\n",
		  abs(gpst[i].lsats[n].num),
		  gpst[i].lsats[n].el,
		  gpst[i].lsats[n].az,
		  gpst[i].lsats[n].sn,
		  gpst[i].lsats[n].num < 0 ?'*':' ');
        sprintf(&xbuf[strlen(xbuf)], "</table>\n</td>\n");
    }

    strcpy(&xbuf[strlen(xbuf)], gpspage9);
}

// self-contained javascript sat html5 canvas using web json
static void dosats()
{
    strcpy(xbuf, satstat);
}

// really primitive parser
void dowebget()
{
    char *c;
    if (strstr(xbuf, "kml"))
        dokml(xbuf);
    else if (strstr(xbuf, "ngjson"))
        dongjson();
    else if (strstr(xbuf, "json"))
        dojson();
    else if (strstr(xbuf, "gpsdata.xml"))
        doxml();
    else if (strstr(xbuf, "dogmap.html"))
        dogmapx();
    else if ((c = strstr(xbuf, "radar"))) {
        c += 5;
        int i = atoi(c);
        if (!i)
            i = 20;             // miles = about 17 px per mile
        int w = 340, h = 340;
        dorad(i, w, h);
    }
    else if ((c = strstr(xbuf, "sats"))) {
        dosats();
    }
    else
        doweb();
}
