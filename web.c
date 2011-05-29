#include "webgpsd.h"

char *radfmt;

// kml network link feed for google earth
static const char gpskml[] = "<Document>"
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
    d = strstr(c, ".kml ");
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
static const char xmldata[] = "<xml><markers><marker lat=\"%d.%06d\" lng=\"%d.%06d\"/></markers></xml>";
static void doxml()
{
    sprintf(xbuf, xmldata, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
      gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000));
}
#ifdef HARLEY

extern struct harley hstat;
// JSON for web - generally NOT gpsd-ng (that will go into the mainline)
static const char hogdata[] =
    "{rpm:%d,speed:%d,full:%d,gear:%d,clutch:%d,neutral:%d,temp:%d,turn:%d,odo:%d,fuel:%d}\r\n";
static void dohog()
{
    sprintf(xbuf, hogdata,
	    hstat.rpm, hstat.vspd, hstat.full, hstat.gear, hstat.clutch, hstat.neutral,
	    hstat.engtemp, hstat.turnsig, hstat.odoaccum, hstat.fuelaccum);
}
#endif
// JSON for web - generally NOT gpsd-ng (that will go into the mainline)
static const char jsondata[] =
  "{lat:%d.%06d,lon:%d.%06d,alt:%d.%03d,pdop:%d.%02d,hdop:%d.%02d,vdop:%d.%02d,track:%d.%03d,speed:%d.%03d,mode:%d,lock:%d,ns:%d,\r\nu:[";
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
      spd / 1000, spd % 1000, gpst[bestgps].fix, gpst[bestgps].lock, gpst[bestgps].pnsats + gpst[bestgps].lnsats);
    if( bestgps < 0 || ( !gpst[bestgps].pnsats && ! gpst[bestgps].lnsats)) {
	 strcat(xbuf, "]}\r\n");
	 return;
    }
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
#if 0
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
    // satellite status
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
#endif

// Wunderground.com severe weather radar, mostly self-contained with refreshing
static void dorad(int size, int picwidth, int picheight)
{
    sprintf(xbuf, radfmt, size / 2, size * 2, gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
      gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000), size, picwidth, picheight);
}

// Standard gps view - menu plus gps source status
static const char gpspage1[] =        // menu
  "<HEAD><TITLE>%s</TITLE><meta http-equiv=\"refresh\" content=\"5\">"
  "<meta name = \"viewport\" content = \"width = 480\">"
  "<style type=\"text/css\">table a {display: block; width: 100%%; height: 100%%}</style>"
  "</HEAD><BODY>\n"
  "<table><tr><td>"
  "\n<table border=1>"
  "<tr><td><a href=/dogmap.html><h1>MAP<br><br></a>"
  "<tr><td><a href=/hogstat.html><h1>HOG<br><br></a>"
  "<tr><td><a href=/satstat.html><h1>SatStat<br><br></a>"
  "<tr><td><a href=/radar20.html><h1>RADAR<br><br></a>" "</table>\n";

static const char gpspage2[] =        // source status
  "<td>\n<table border=%d><tr><td>"
  "%02d-%02d-%02d %02d:%02d:%02d<tr><td>"
  "lat=%d.%06d<tr><td>lon=%d.%06d<tr><td>"
  "%dD fix<tr><td>%s lock<tr><td>"
  "alt=%d.%03d<tr><td>"
  "spd=%d.%03d<tr><td>"
  "trk=%d.%03d<tr><td>" "PDoP=%d.%02d<tr><td>" "HDoP=%d.%02d<tr><td>" "VDoP=%d.%02d</table>\n"
  "<td>\n<table border=%d><tr><th>PRN<th>Elev<th>Azim<th>Sgnl<th>Usd</tr>";

static const char gpspage9[] =        // closing
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

// really primitive parser
int dowebget()
{
    char *c;
    if (strstr(xbuf, "gpsdata") && strstr(xbuf, ".kml "))
        dokml(xbuf);
#if 0
    else if (strstr(xbuf, "ngjson"))
        dongjson();
#endif
#ifdef HARLEY
    else if (strstr(xbuf, "hogstat.json"))
        dohog();
#endif
    else if (strstr(xbuf, "gpsstat.json"))
        dojson();
    else if (strstr(xbuf, "gpsdata.xml"))
        doxml();
    else if ((c = strstr(xbuf, "radar"))) {
        c += 5;
        int i = atoi(c);
        if (!i)
            i = 20;             // miles = about 17 px per mile
        int w = 340, h = 340;
        dorad(i, w, h);
    }
    else {
	extern char *webdirprefix;
	char ibuf[256];
	strncpy( ibuf, webdirprefix, 256 );
	ibuf[255] = 0;
	strcat( ibuf, "/" );
	do {
	    char *c = strchr( xbuf, ' ' );
	    if( !c )
		break;
	    char *d = strchr( ++c, ' ' );
	    if( !*d )
		break;
	    *d = 0;
	    strcat( ibuf, c );
	    int n = open( ibuf, O_RDONLY );
	    if( n < 0 )
		break;
	    int i = lseek( n, 0, SEEK_END ); 
	    lseek( n, 0, SEEK_SET ); 
	    if( i > BUFSIZ )
		break;
	    if( i != read( n, xbuf, i ) ) {
		close(n);
		break;
	    }
	    xbuf[i] = 0;
	    close(n);
	    return 1;
	} while(0);

        doweb();
    }
    return 1;
}
