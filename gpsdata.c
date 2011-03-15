#include "webgpsd.h"

static char kmlname[80] = "";

//kml template

static char kmlhead[] =
  "<Document>\n"
  "<name>%s</name>\n"
  "<LookAt>"
  "<longitude>%d.%06d</longitude>\n"
  "<latitude>%d.%06d</latitude>\n"
  "<range>500</range>"
  "<tilt>60</tilt>"
  "<altitude>0</altitude>"
  "<altitudeMode>relativeToGround</altitudeMode>"
  "<heading>%d.%03d</heading>"
  "</LookAt>"
  "<Style id=\"x\"><LineStyle><width>2</width><color>ff0000ff</color></LineStyle>"
  "<PolyStyle><color>33000000</color></PolyStyle></Style>\n" "<Placemark><LineString><coordinates>\n";

static char pmarkfmt[] =
  "%d.%06d,%d.%06d,%d.%03d\n"
  "</coordinates></LineString></Placemark>\n"
  "<Placemark>\n"
  "<TimeStamp><when>20%02d-%02d-%02dT%02d:%02d:%02dZ</when></TimeStamp>\n"
  "<styleUrl>#x</styleUrl><LineString><extrude>1</extrude>" "<altitudeMode>relativeToGround</altitudeMode><coordinates>\n";

static char kmltail[] = "</coordinates></LineString></Placemark>\n</Document>\n";

//NMEA field data extraction helpers

static char *field[100];        // expanded to 100 for G-Rays PUBX03

void addnmeacksum(char *c)
{
    int i = 0;
    char *d = c;
    d += strlen(d);
    *d++ = '*';
    *d = 0;
    c++;
    while (*c && *c != '*')
        i ^= *c++;
    i &= 0xff;
    sprintf(++c, "%02X", i);
}

static int get2(char *c)
{
    int i = 0;
    if (*c)
        i = (*c++ - '0') * 10;
    if (*c)
        i += *c - '0';
    return i;
}

static int get3(char *c)
{
    int i = 0;
    if (*c)
        i = (*c++ - '0') * 100;
    i += get2(c);
    return i;
}

static int get3dp(int f)
{
    int i = 0;
    char *d = field[f];
    while (*d && *d != '.') {
        i *= 10;
        i += (*d++ - '0');
    }
    i *= 1000;
    if (*d == '.')
        d++;

    if (*d)
        i += (*d++ - '0') * 100;
    if (*d)
        i += (*d++ - '0') * 10;
    if (*d)
        i += *d++ - '0';
    return i;
}

static int get0dp(int f)
{
    int i = 0;
    char *d = field[f];
    while (*d && *d != '.') {
        i *= 10;
        i += (*d++ - '0');
    }
    return i;
}

static int cidx = 0;
static void gethms(int i)
{
    //hms field[i]
    char *c = field[i];
    gpst[cidx].hr = get2(c);
    gpst[cidx].mn = get2(&c[2]);
    gpst[cidx].sc = get2(&c[4]);
    if (c[6] && c[6] == '.')
        gpst[cidx].scth = get3(&c[7]);
}

static int getminutes(char *d)
{
    int i;
    i = (*d++ - '0') * 1000000;
    //Minutes with decimal
    i += (*d++ - '0') * 100000;
    if (*d)
        d++;
    if (*d)
        i += (*d++ - '0') * 10000;
    if (*d)
        i += (*d++ - '0') * 1000;
    if (*d)
        i += (*d++ - '0') * 100;
    if (*d)
        i += (*d++ - '0') * 10;
    return (i + 1) / 6;
}

static void getll(int f)
{
    int l, d;
    char *c;

    c = field[f++];
    l = get2(c);
    c += 2;
    d = getminutes(c);

    c = field[f++];
    l *= 1000000;
    l += d;
    if (*c != 'N')
        l = -l;
    //    if (l != gpst[cidx].llat)
    //        chg = 1;
    gpst[cidx].llat = l;

    c = field[f++];
    l = get3(c);
    c += 3;
    d = getminutes(c);

    c = field[f];

    l *= 1000000;
    l += d;
    if (*c != 'E')
        l = -l;
    //    if (l != gpst[cidx].llon)
    //        chg = 1;
    gpst[cidx].llon = l;

}

//KML Logging
static char kmlstr[BUFLEN];
static int kmlful = 0;

// append data, close KML, and rewind redy for next data
static void dokmltail()
{
    if (!logfd)
        return;
    strcpy(&kmlstr[kmlful], kmltail);
    fputs(kmlstr, logfd);
    kmlful = 0;
    kmlstr[0] = 0;
    fflush(logfd);
    fseek(logfd, -strlen(kmltail), SEEK_CUR);
}

void add2kml(char *add)
{
    strcpy(&kmlstr[kmlful], add);
    kmlful += strlen(add);
    if (kmlful > BUFLEN / 2)
        dokmltail();
}

// Save and zip current KML
extern int kmlzipper( char *kmlfn );
static void kmzip(char *fname)
{
    fprintf(errfd, "Zipping %s\n", fname);
    if (!fork()) {
        int k, n = getdtablesize();
        for( k = 3 ; k < n ; k++ )
            close(k);
        exit( kmlzipper( fname ) );
    }
}

void rotatekml()
{
    char lbuf[256];
    struct timeval tv;
    struct tm *tmpt;

    // use syslock to avoid collisions and nonlocked time errors
    gettimeofday(&tv, NULL);
    tmpt = gmtime(&tv.tv_sec);

    sprintf(lbuf, "%02d%02d%02d%02d%02d%02d.kml",
      tmpt->tm_year % 100, 1 + tmpt->tm_mon, tmpt->tm_mday, tmpt->tm_hour, tmpt->tm_min, tmpt->tm_sec);

    // added gpst[cidx].mn++ to teardown - need to test
    if (!strcmp(lbuf, kmlname))
        lbuf[strlen(lbuf) - 5]++;       // increase minutes to avoid overwrite
    // normally only happens at end when sigint closes too quickly.
    strcpy(kmlname, lbuf);
    if (logfd) {
        dokmltail();            // write out anything remaining in buffer
        fclose(logfd);
        if (rename("current.kml", lbuf)) {      // no current.kml yet, starting
            lbuf[10] = 0;
            strcat(lbuf, ".lastcur.kml");
            rename("../prevcur.kml", lbuf);
            rename("prevcur.kml", lbuf);
            kmzip(lbuf);
            lbuf[10] = 0;
            strcat(lbuf, ".prlock.kml");
            //      if( ftell(logfd) <= strlen(kmltail) * 2 ) {} 
// if no data, don't rename prelock
            rename("../prlock.kml", lbuf);
            rename("prlock.kml", lbuf);
        }
        kmzip(lbuf);
    }
}

// sync internal lock on first lock
static int firslock = 0;
static void writelock()
{
    firslock = 1;
#if 0
    char cmd[256];
    int i;
    // set system lock - linux generic
    // sprintf( cmd, "sudo date -u -s %02d/%02d/20%02d", gpst[cidx].mo,gpst[cidx].dy,gpst[cidx].yr );
    // sprintf( cmd, "sudo date -u -s %02d:%02d:%02d", gpst[cidx].hr,gpst[cidx].mn,gpst[cidx].sc );
    // sprintf( cmd, "sudo hwlock --systohc" );

#ifdef NOKIAN810
    // nokia
    if (!access("/mnt/initfs/usr/bin/retutime", F_OK)) {
        sprintf(cmd, "sudo /usr/sbin/chroot /mnt/initfs /usr/bin/retutime -T 20%02d-%02d-%02d/%02d:%02d:%02d",
          gpst[cidx].yr, gpst[cidx].mo, gpst[cidx].dy, gpst[cidx].hr, gpst[cidx].mn, gpst[cidx].sc);
        i = system(cmd);
        //  fprintf( errfd, "Set Time %d=%s\n", i, cmd );
        system("sudo /usr/sbin/chroot /mnt/initfs /usr/bin/retutime -i");       // update system from RTC
    }
#endif
#endif
}

// process NMEA to set data

static int kmmn = -1;
static int kmlsc = -1;
static int kmscth = -1;

// expects single null terminated strings (line ends dont matter)
int getgpsinfo(int chan, char *buf, int msclock)
{
    char *c, *d;
    int i, fmax;

    c = buf;

    d = NULL;
    // required for pathologic cases of $GPABC...$GPXYZ...*ck 
    // where $GPABC... resolves to zero
    for (;;) {                  // find last $ - start of NMEA
        c = strchr(c, '$');
        if (!c)
            break;
        d = c;
        c++;
    }
    if (!d)
        return 0;

    // ignore all but standard NMEA
    if (*d != '$' )
        return 0;
    if (d[1] != 'G' )
        return 0;
    if (d[2] != 'P' && d[2] != 'N' )
        return 0;

    c = d;
    c++;

    //verify checksum
    i = 0;
    while (*c && *c != '*')
        i ^= *c++;
    if (!*c || (unsigned) (i & 0xff) != strtoul(++c, NULL, 16)) {
        fprintf(errfd, "Bad NMEA Checksum, calc'd %02x:\n %s", i, d);
        return -1;
    }
    --c;
    //null out asterisk
    *c = 0;

    c = d;

    //find and update timestamp
    for( cidx = 0 ; cidx < MAXSRC; cidx++ )
        if( gpst[cidx].gpsfd == chan || gpst[cidx].gpsfd == -2 )  // found or EOL
            break;
    if( cidx == MAXSRC ) // full, find empty or stale
        for( cidx = 0 ; cidx < MAXSRC; cidx++ )
            if( gpst[cidx].gpsfd < 0 || (100000 + msclock - gpst[cidx].lastseen) % 100000 > 1250 )
                break;
    gpst[cidx].gpsfd = chan;
    gpst[cidx].lastseen = msclock;

    //Split into fields at the commas
    fmax = 0;
    c+=3;
    for (;;) {
        field[fmax++] = c;
        c = strchr(c, ',');
        if (c == NULL)
            break;
        *c++ = 0;
    }

    //Latitude, Longitude, and other info
    if (fmax == 13 && !strcmp(field[0], "RMC")) {
        //NEED TO VERIFY FMAX FOR EACH
        if (field[2][0] != 'A') {
            if( gpst[cidx].lock )
                gpst[cidx].lock = 0;
            return 1;
        } else {
            if( !gpst[cidx].lock )
                gpst[cidx].lock = 1;
            gethms(1);
            getll(3);
            gpst[cidx].gspd = get3dp(7) * 1151 / 1000;
            //convert to MPH
            gpst[cidx].gtrk = get3dp(8);
            //Date, DDMMYY
            gpst[cidx].dy = get2(field[9]);
            gpst[cidx].mo = get2(&field[9][2]);
            gpst[cidx].yr = get2(&field[9][4]);

            // this will be slightly late
            if (!firslock)
                writelock();
        }
    } else if (fmax == 15 && !strcmp(field[0], "GGA")) {
        i = field[6][0] - '0';
	// was gpst[cidx].lock, but it would prevent GPRMC alt
        if (!i)
            return 1;
        else
            if( gpst[cidx].lock != i )
                gpst[cidx].lock = i;
        // Redundant: getll(2);
        // don't get this here since it won't increment the YMD
        // and create a midnight bug
        //       gethms(1);
        //7 - 2 plc Sats Used
        // 8 - HDOP
        gpst[cidx].hdop = get3dp(8);
        gpst[cidx].alt = get3dp(9);
        //9, 10 - Alt, units M
    }
#if 0 // depend on RMC to avoid midnight bugs
    else if (fmax == 8 && !strcmp(field[0], "GLL")) {
        if (field[6][0] != 'A') {
#if 0       // this will cause problems for the kml rotate if the time is wrong
            if (strlen(field[5]))
                gethms(5);
#endif
           if( gpst[cidx].lock )
                gpst[cidx].lock = 0;
            return 1;
        }
        if( !gpst[cidx].lock )
            gpst[cidx].lock = 1;
        getll(1);
        gethms(5);
    }
#endif
#if 0
    else if (fmax == 10 && !strcmp(field[0], "VTG")) {
        gpst[cidx].gtrk = get3dp(1);
        gpst[cidx].gspd = get3dp(5) * 1151 / 1000;
        //convert to MPH
    }
#endif
    //Satellites and status
    else if (!(fmax & 3) && fmax >= 8 && fmax <= 20 && !strcmp(field[0], "GSV")) {
        int j, tot, seq, viewcnt;
        //should check (fmax % 4 == 3)
        tot = get0dp(1);
        seq = get0dp(2);
        viewcnt = 4 * (seq - 1);
        gpsat[cidx].nsats = get0dp(3);
        for (j = 4; j < 20 && j < fmax; j += 4) {
            i = get0dp(j);
            if (!i)
                return 1;
            gpsat[cidx].view[viewcnt++] = i;
            gpsat[cidx].el[i] = get0dp(j + 1);
            gpsat[cidx].az[i] = get0dp(j + 2);
            gpsat[cidx].sn[i] = get0dp(j + 3);
        }
        gpsat[cidx].satset &= (1 << tot) - 1;
        gpsat[cidx].satset &= ~ (1 << (seq-1));
	if( !gpsat[cidx].satset ) {
	    int n , m, k;
	    gpst[cidx].nsats = gpsat[cidx].nsats;
	    gpst[cidx].nused = gpsat[cidx].nused;
	    for (n = 0; n < gpsat[cidx].nsats; n++) {
	        m = gpsat[cidx].view[n];
		gpst[cidx].sats[n].num = m;
		gpst[cidx].sats[n].el = gpsat[cidx].el[m];
		gpst[cidx].sats[n].az = gpsat[cidx].az[m];
		gpst[cidx].sats[n].sn = gpsat[cidx].sn[m];
		for (k = 0; k < 12; k++)
		    if (gpsat[cidx].sats[k] == m)
		        break;
		if( k < 12 )
		    gpst[cidx].sats[n].num = -m;
	    }
	}
    } else if (fmax == 18 && !strcmp(field[0], "GSA")) {
        gpsat[cidx].satset = 255;
        gpst[cidx].fix = get0dp(2);
	gpsat[cidx].nused = 0;
        for (i = 3; i < 15; i++) {
            gpsat[cidx].sats[i] = get0dp(i);
	    if( gpsat[cidx].sats[i] )
	        gpsat[cidx].nused++;
	    // else break;?
	}
        gpst[cidx].pdop = get3dp(15);
        gpst[cidx].hdop = get3dp(16);
        gpst[cidx].vdop = get3dp(17);
    }
#if 0
    else
        printf("?%s\n", field[0]);
#endif
    if( bestgps != cidx )
        return 1;

    if (!gpst[cidx].mo || !gpst[cidx].dy)
        return 1;
    // within 24 hours, only when gpst[cidx].lock since two unlocked GPS can have different times
    // only when sc < 30 to avoid bestgps jitter (5:00->4:59) causing a hiccup
    if (kmlinterval && gpst[cidx].lock && gpst[cidx].sc < 30 
        && kmmn != (gpst[cidx].hr * 60 + gpst[cidx].mn) / kmlinterval) {
        kmmn = (gpst[cidx].hr * 60 + gpst[cidx].mn) / kmlinterval;
        rotatekml();
        logfd = fopen("current.kml", "w+b");
	if( logfd ) {
        fprintf(logfd, kmlhead, kmlname, gpst[cidx].llon / 1000000, abs(gpst[cidx].llon % 1000000), 
                gpst[cidx].llat / 1000000, abs(gpst[cidx].llat % 1000000), gpst[cidx].gtrk / 1000,
          gpst[cidx].gtrk % 1000);
        fflush(logfd);
	}
    }
    if (kmlsc == gpst[cidx].sc && kmscth == gpst[cidx].scth)
        return 1;
    if (!gpst[cidx].llat && !gpst[cidx].llon)         // time, but no location
        return 1;

    if (kmlsc != gpst[cidx].sc) {
	if( !kmlinterval || !logfd )
		return 1;

        kmlsc = gpst[cidx].sc;

        //sprint then fputs in dokmltail to make it a unitary write
        //(otherwise current.kml may be read as a partial)
        sprintf(&kmlstr[kmlful], pmarkfmt, gpst[cidx].llon / 1000000, abs(gpst[cidx].llon % 1000000),
                gpst[cidx].llat / 1000000, abs(gpst[cidx].llat % 1000000), gpst[cidx].gspd / 1000, gpst[cidx].gspd % 1000,  // first and last
          gpst[cidx].yr, gpst[cidx].mo, gpst[cidx].dy, gpst[cidx].hr, gpst[cidx].mn, gpst[cidx].sc);
        kmlful = strlen(kmlstr);
    }

    kmscth = gpst[cidx].scth;
    sprintf(&kmlstr[kmlful], "%d.%06d,%d.%06d,%d.%03d\n", gpst[cidx].llon / 1000000, abs(gpst[cidx].llon % 1000000), 
            gpst[cidx].llat / 1000000, abs(gpst[cidx].llat % 1000000), gpst[cidx].gspd / 1000, gpst[cidx].gspd % 1000);
    kmlful = strlen(kmlstr);

    if (kmlful > BUFLEN / 2)
        dokmltail();
    return 2;
}

void findbestgps() {
    // find size of list
    for( cidx = 0 ; cidx < MAXSRC; cidx++ )
        if( gpst[cidx].gpsfd == -2 )  // found or EOL
            break;

    while( cidx > 0 && gpst[cidx - 1].gpsfd == -1 )
        gpst[--cidx].gpsfd = -2; // shorten table if possible

    if( !cidx )
        return;

    int i, mxlock = gpst[bestgps].lock, mxfix= gpst[bestgps].fix, mnpdop=9999, currbest = bestgps;
    // empty stale entries
    for( i = 0 ; i < cidx; i++ ) {
        if( gpst[i].gpsfd >= 0 && (100000 + thisms - gpst[i].lastseen) % 100000 > 1250 ) {
            memset( &gpst[i], 0, sizeof(gpst[i]) ); // clear to avoid stale data when back online
            memset( &gpsat[i], 0, sizeof(gpsat[i]) );
            gpst[i].gpsfd = -1; // stale
            continue;
        }
        if( gpst[i].lock < mxlock )
            continue;
        if( gpst[i].lock > mxlock ) { // prefer dgps/sbas
            currbest = i;
            mxlock = gpst[i].lock;
            mxfix = gpst[i].fix;
            continue;
        }
        // equal locks, check fix
        if( gpst[i].fix < mxfix )
            continue;
        if( gpst[i].fix > mxfix ) {
            currbest = i;
            mxfix = gpst[i].fix;
            mnpdop = gpst[i].pdop;
            continue;
        }
        if( gpst[i].pdop >= mnpdop )
            continue;
        mnpdop = gpst[i].pdop;
        // better fix, better pdop
        currbest = i;
    }
    bestgps = currbest;
}
