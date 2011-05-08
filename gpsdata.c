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

unsigned int tenexp[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

static int getndp(char *d, int p)
{
    int i = 0;
    while (*d && *d != '.') {
        i *= 10;
        i += (*d++ - '0');
    }
    if (!p)
        return i;

    i *= tenexp[p];

    if (*d == '.')
        d++;
    while (*d && p--)
        i += (*d++ - '0') * tenexp[p];  // p == 0 can be optimized

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

static void getll(int f)
{
    int l, d;
    char *c;

    c = field[f++];
    l = get2(c);
    c += 2;
    d = (getndp(c, 5) + 1) / 6;

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
    d = (getndp(c, 5) + 1) / 6;

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
extern int kmlzipper(char *kmlfn);
extern char *cmdname;
static void kmzip(char *fname)
{
    fprintf(errfd, "Zipping %s\n", fname);
    if (!fork()) {
	cmdname = "webgpsd-kmltokmz"; 
        int k, n = getdtablesize();
        for (k = 3; k < n; k++)
            close(k);
        exit(kmlzipper(fname));
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
    if (*d != '$')
        return 0;
    if (d[1] != 'G')
        return 0;
    if (d[2] != 'P' && d[2] != 'N' && d[2] != 'L')
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
    for (cidx = 0; cidx < MAXSRC; cidx++)
        if (gpst[cidx].gpsfd == chan || gpst[cidx].gpsfd == -2) // found or EOL
            break;
    if (cidx == MAXSRC)         // full, find empty or stale
        for (cidx = 0; cidx < MAXSRC; cidx++)
            if (gpst[cidx].gpsfd < 0 || (100000 + msclock - gpst[cidx].lastseen) % 100000 > 1250)
                break;
    gpst[cidx].gpsfd = chan;
    gpst[cidx].lastseen = msclock;

    //Split into fields at the commas
    fmax = 0;
    c += 2;
    char satype = *c++;         // P,L,N
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
            if (gpst[cidx].lock)
                gpst[cidx].lock = 0;
            return 1;
        }
        else {
            if (!gpst[cidx].lock)
                gpst[cidx].lock = 1;
            gethms(1);
            getll(3);
            gpst[cidx].gspd = getndp(field[7], 3) * 1151 / 1000;
            //convert to MPH
            gpst[cidx].gtrk = getndp(field[8], 3);
            //Date, DDMMYY
            gpst[cidx].dy = get2(field[9]);
            gpst[cidx].mo = get2(&field[9][2]);
            gpst[cidx].yr = get2(&field[9][4]);

            // this will be slightly late
            if (!firslock)
                writelock();
        }
    }
    else if (fmax == 15 && (!strcmp(field[0], "GGA") || !strcmp(field[0], "GNS"))) {
        i = field[6][0] - '0';
        // was gpst[cidx].lock, but it would prevent GPRMC alt
        if (!i)
            return 1;
        else if (gpst[cidx].lock != i)
            gpst[cidx].lock = i;
        // Redundant: getll(2);
        // don't get this here since it won't increment the YMD
        // and create a midnight bug
        //       gethms(1);
        //7 - 2 plc Sats Used
        // 8 - HDOP
        gpst[cidx].hdop = getndp(field[8], 3);
        gpst[cidx].alt = getndp(field[9], 3);
        //9, 10 - Alt, units M
    }
#if 0                           // depend on RMC to avoid midnight bugs
    else if (fmax == 8 && !strcmp(field[0], "GLL")) {
        if (field[6][0] != 'A') {
#if 0                           // this will cause problems for the kml rotate if the time is wrong
            if (strlen(field[5]))
                gethms(5);
#endif
            if (gpst[cidx].lock)
                gpst[cidx].lock = 0;
            return 1;
        }
        if (!gpst[cidx].lock)
            gpst[cidx].lock = 1;
        getll(1);
        gethms(5);
    }
#endif
#if 0
    else if (fmax == 10 && !strcmp(field[0], "VTG")) {
        gpst[cidx].gtrk = getndp(field[1], 3);
        gpst[cidx].gspd = getndp(field[5], 3) * 1151 / 1000;
        //convert to MPH
    }
#endif
    //Satellites and status
    else if (!(fmax & 3) && fmax >= 8 && fmax <= 20 && !strcmp(field[0], "GSV")) {
        int j, tot, seq;
        //should check (fmax % 4 == 3)
        tot = getndp(field[1], 0);
        seq = getndp(field[2], 0);
        if (satype == 'P') {
            if (seq == 1)
                for (j = 0; j < 65; j++)
                    gpsat[cidx].view[j] = 0;
            gpsat[cidx].pnsats = getndp(field[3], 0);
            gpsat[cidx].psatset &= (1 << tot) - 1;
            gpsat[cidx].psatset &= ~(1 << (seq - 1));
        }
        else {
            if (seq == 1)
                for (j = 65; j < 100; j++)
                    gpsat[cidx].view[j] = 0;
            gpsat[cidx].lnsats = getndp(field[3], 0);
            gpsat[cidx].lsatset &= (1 << tot) - 1;
            gpsat[cidx].lsatset &= ~(1 << (seq - 1));
        }

        for (j = 4; j < 20 && j < fmax; j += 4) {
            i = getndp(field[j], 0);
            if (!i)
                break;
	    if( i > 119 ) // WAAS,EGNOS high numbering
	      i -= 87;
            gpsat[cidx].view[i] = 1;
            gpsat[cidx].el[i] = getndp(field[j + 1], 0);
            gpsat[cidx].az[i] = getndp(field[j + 2], 0);
            gpsat[cidx].sn[i] = getndp(field[j + 3], 0);
        }
        int n;
        if (satype == 'P' && !gpsat[cidx].psatset) {
            gpst[cidx].pnsats = 0;
            gpst[cidx].pnused = 0;
            for (n = 0; n < 65; n++) {
                if (gpsat[cidx].view[n]) {
                    int k = gpst[cidx].pnsats++;
                    gpst[cidx].psats[k].num = n;
                    gpst[cidx].psats[k].el = gpsat[cidx].el[n];
                    gpst[cidx].psats[k].az = gpsat[cidx].az[n];
                    gpst[cidx].psats[k].sn = gpsat[cidx].sn[n];
                    if (gpsat[cidx].used[n]) {
                        gpst[cidx].pnused++;
                        gpst[cidx].psats[k].num = -n;
                    }
                    else
                        gpst[cidx].psats[k].num = n;
                }
            }
        }
        // else
        if (satype == 'L' && !gpsat[cidx].lsatset) {
            gpst[cidx].lnsats = 0;
            gpst[cidx].lnused = 0;
            for (n = 65; n < 99; n++) {
                if (gpsat[cidx].view[n]) {
                    int k = gpst[cidx].lnsats++;
                    gpst[cidx].lsats[k].num = n;
                    gpst[cidx].lsats[k].el = gpsat[cidx].el[n];
                    gpst[cidx].lsats[k].az = gpsat[cidx].az[n];
                    gpst[cidx].lsats[k].sn = gpsat[cidx].sn[n];
                    if (gpsat[cidx].used[n]) {
                        gpst[cidx].lnused++;
                        gpst[cidx].lsats[k].num = -n;
                    }
                    else
                        gpst[cidx].lsats[k].num = n;
                }
            }
        }
    }
    else if (fmax == 18 && !strcmp(field[0], "GSA")) {
        gpst[cidx].fix = getndp(field[2], 0);
        gpst[cidx].pdop = getndp(field[15], 3);
        gpst[cidx].hdop = getndp(field[16], 3);
        gpst[cidx].vdop = getndp(field[17], 3);

        int j = getndp(field[3], 0);
	if( j > 119 )
	    j -= 87;
        if (j && j < 65) {
            gpsat[cidx].psatset = 255;
            for (i = 0; i < 65; i++)
                gpsat[cidx].used[i] = 0;
            gpsat[cidx].pnused = 0;
            for (i = 3; i < 15; i++) {
                int k = getndp(field[i], 0);
		if( k > 119 )
		  k -= 87;
                if (k) {
                    gpsat[cidx].used[k]++;
                    gpsat[cidx].pnused++;
                }
                // else break;?
            }
        }
        if (j && j > 64) {
            gpsat[cidx].lsatset = 255;
            for (i = 65; i < 100; i++)
                gpsat[cidx].used[i] = 0;
            gpsat[cidx].lnused = 0;
            for (i = 3; i < 15; i++) {
                int k = getndp(field[i], 0);
		if( k > 119 )
		  k -= 87;
                if (k) {
                    gpsat[cidx].used[k]++;
                    gpsat[cidx].lnused++;
                }
                // else break;?
            }
        }
    }
#if 0
    else
        printf("?%s\n", field[0]);
#endif
    if (bestgps != cidx)
        return 1;

    if (!gpst[cidx].mo || !gpst[cidx].dy)
        return 1;
    // within 24 hours, only when gpst[cidx].lock since two unlocked GPS can have different times
    // only when sc < 30 to avoid bestgps jitter (5:00->4:59) causing a hiccup
    if (kmlinterval && gpst[cidx].lock && gpst[cidx].sc < 30 && kmmn != (gpst[cidx].hr * 60 + gpst[cidx].mn) / kmlinterval) {
        kmmn = (gpst[cidx].hr * 60 + gpst[cidx].mn) / kmlinterval;
        rotatekml();
        logfd = fopen("current.kml", "w+b");
        if (logfd) {
            fprintf(logfd, kmlhead, kmlname, gpst[cidx].llon / 1000000, abs(gpst[cidx].llon % 1000000),
              gpst[cidx].llat / 1000000, abs(gpst[cidx].llat % 1000000), gpst[cidx].gtrk / 1000, gpst[cidx].gtrk % 1000);
            fflush(logfd);
        }
    }
    if (kmlsc == gpst[cidx].sc && kmscth == gpst[cidx].scth)
        return 1;
    if (!gpst[cidx].llat && !gpst[cidx].llon)   // time, but no location
        return 1;

    if (kmlsc != gpst[cidx].sc) {
        if (!kmlinterval || !logfd)
            return 1;

        kmlsc = gpst[cidx].sc;

        //sprint then fputs in dokmltail to make it a unitary write
        //(otherwise current.kml may be read as a partial)
        sprintf(&kmlstr[kmlful], pmarkfmt, gpst[cidx].llon / 1000000, abs(gpst[cidx].llon % 1000000), gpst[cidx].llat / 1000000, abs(gpst[cidx].llat % 1000000), gpst[cidx].gspd / 1000, gpst[cidx].gspd % 1000,        // first and last
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

void findbestgps()
{
    // find size of list
    for (cidx = 0; cidx < MAXSRC; cidx++)
        if (gpst[cidx].gpsfd == -2)     // found or EOL
            break;

    while (cidx > 0 && gpst[cidx - 1].gpsfd == -1)
        gpst[--cidx].gpsfd = -2;        // shorten table if possible

    if (!cidx)
        return;

    int i, mxlock = gpst[bestgps].lock, mxfix = gpst[bestgps].fix, mnpdop = 9999, currbest = bestgps;
    // empty stale entries
    for (i = 0; i < cidx; i++) {
        if (gpst[i].gpsfd >= 0 && (100000 + thisms - gpst[i].lastseen) % 100000 > 1250) {
            memset(&gpst[i], 0, sizeof(gpst[i]));       // clear to avoid stale data when back online
            memset(&gpsat[i], 0, sizeof(gpsat[i]));
            gpst[i].gpsfd = -1; // stale
            continue;
        }
        if (gpst[i].lock < mxlock)
            continue;
        if (gpst[i].lock > mxlock) {    // prefer dgps/sbas
            currbest = i;
            mxlock = gpst[i].lock;
            mxfix = gpst[i].fix;
            continue;
        }
        // equal locks, check fix
        if (gpst[i].fix < mxfix)
            continue;
        if (gpst[i].fix > mxfix) {
            currbest = i;
            mxfix = gpst[i].fix;
            mnpdop = gpst[i].pdop;
            continue;
        }
        if (gpst[i].pdop >= mnpdop)
            continue;
        mnpdop = gpst[i].pdop;
        // better fix, better pdop
        currbest = i;
    }
    // should add weighting and hysteresis
    bestgps = currbest;
}
