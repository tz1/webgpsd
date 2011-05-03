// CONFIG
static char *pidlockfile = "/tmp/webgpsd.pid";
static char *logdirprefix = "/tmp/";
int kmlinterval = 5;
static int gpsdport = 2947;

#include "webgpsd.h"
#include <errno.h>
#include <sys/termios.h>

int bestgps = 0;
struct gpsstate gpst[MAXSRC];
struct gpssats gpsat[MAXSRC];

FILE *errfd = NULL;
FILE *logfd = NULL;
char *xbuf;

extern void add2kml(char *);

static int acpt[MAXCONN];
static int raw[MAXCONN];
static int watch[MAXCONN];
static int amax = 0;

//need conn to toggle raw for it 
static void prtgpsinfo(int conn, char *c)
{
    char cbuf[256];
    int n;
    struct timeval tv;
    strcpy(xbuf, "GPSD");
    while (*c) {
        if (*c >= 'a')
            *c -= 32;
        switch (*c) {
            // bauds were here 
        case 'P':
            sprintf(cbuf, ",P=%d.%06d %d.%06d", gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
              gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000));
            break;
        case 'A':
            sprintf(cbuf, ",A=%d.%03d", gpst[bestgps].alt / 1000, abs(gpst[bestgps].alt % 1000));
            break;
        case 'D':
            sprintf(cbuf, ",D=%02d-%02d-%02d %02d:%02d:%02d", gpst[bestgps].yr, gpst[bestgps].mo, gpst[bestgps].dy,
              gpst[bestgps].hr, gpst[bestgps].mn, gpst[bestgps].sc);
            break;
        case 'V':
            sprintf(cbuf, ",V=%d.%03d", gpst[bestgps].gspd / 1000, gpst[bestgps].gspd % 1000);
            break;
        case 'S':
            sprintf(cbuf, ",S=%d", ! !gpst[bestgps].lock);
            break;
        case 'M':
            sprintf(cbuf, ",M=%d", gpst[bestgps].fix);
            break;
        case 'O':
            gettimeofday(&tv, NULL);
            int mpspd = gpst[bestgps].gspd * 447 / 1000;        // mph to m/s
            sprintf(cbuf, ",O=RMC %d.%02d 0.0 %d.%06d %d.%06d %d.%03d %d.%02d %d.%02d %d.%03d %d.%03d 0.0 ? 0.0 ? %d",
              (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec / 10000,
              gpst[bestgps].llat / 1000000, abs(gpst[bestgps].llat % 1000000),
              gpst[bestgps].llon / 1000000, abs(gpst[bestgps].llon % 1000000),
              gpst[bestgps].alt / 1000, abs(gpst[bestgps].alt % 1000),
              gpst[bestgps].hdop / 1000, gpst[bestgps].hdop % 1000 / 10,
              gpst[bestgps].vdop / 1000, gpst[bestgps].vdop % 1000 / 10,
              gpst[bestgps].gtrk / 1000, gpst[bestgps].gtrk % 1000, mpspd / 1000, mpspd % 1000, gpst[bestgps].fix);
            break;
        case 'R':
            raw[conn] = !raw[conn];
            sprintf(cbuf, ",R=%d", raw[conn]);
            break;
        case 'W':
            if (c[1] == '2') {
                c++;
                watch[conn] = 2;
            }
            else if (c[1] == '+' || c[1] == '1') {
                c++;
                watch[conn] = 1;
            }
            else if (c[1] == '-' || c[1] == '0') {
                c++;
                watch[conn] = 0;
            }
            else
                watch[conn] = !watch[conn];
            sprintf(cbuf, ",W=%d", watch[conn]);
            break;
        case 'E':
            sprintf(cbuf, ",E=%d.%02d %d.%02d %d.%02d", gpst[bestgps].pdop / 1000, gpst[bestgps].pdop % 1000 / 10,
              gpst[bestgps].hdop / 1000, gpst[bestgps].hdop % 1000 / 10, gpst[bestgps].vdop / 1000, gpst[bestgps].vdop % 1000 / 10);
            break;
        case 'H':
            sprintf(cbuf, ",H=%d.%03d", gpst[bestgps].gtrk / 1000, gpst[bestgps].gtrk % 1000);
            break;
        case 'J':
            strcat(xbuf, "\r\nU Nu El Azm Sg");
            for (n = 0; n < gpst[bestgps].pnsats; n++) {
                sprintf(cbuf, "\r\n%d %02d %02d %03d %02d", gpst[bestgps].psats[n].num < 0, abs(gpst[bestgps].psats[n].num),
                  gpst[bestgps].psats[n].el, gpst[bestgps].psats[n].az, gpst[bestgps].psats[n].sn);
                strcat(xbuf, cbuf);
            }
            for (n = 0; n < gpst[bestgps].lnsats; n++) {
                sprintf(cbuf, "\r\n%d %02d %02d %03d %02d", gpst[bestgps].lsats[n].num < 0, abs(gpst[bestgps].lsats[n].num),
                  gpst[bestgps].lsats[n].el, gpst[bestgps].lsats[n].az, gpst[bestgps].lsats[n].sn);
                strcat(xbuf, cbuf);
            }
            cbuf[0] = 0;
            break;
        case 'Y':
            gettimeofday(&tv, NULL);
            sprintf(cbuf, "GPSD,Y=GSV %d.%06d %d:", (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec,
              gpst[bestgps].pnsats + gpst[bestgps].lnsats);
            strcat(xbuf, cbuf);
            for (n = 0; n < gpst[bestgps].pnsats; n++) {
                sprintf(cbuf, "%d %d %d %d %d:", abs(gpst[bestgps].psats[n].num), gpst[bestgps].psats[n].el,
                  gpst[bestgps].psats[n].az, gpst[bestgps].psats[n].sn, gpst[bestgps].psats[n].num < 0);
                strcat(xbuf, cbuf);
                cbuf[0] = 0;
            }
            for (n = 0; n < gpst[bestgps].lnsats; n++) {
                sprintf(cbuf, "%d %d %d %d %d:", abs(gpst[bestgps].lsats[n].num), gpst[bestgps].lsats[n].el,
                  gpst[bestgps].lsats[n].az, gpst[bestgps].lsats[n].sn, gpst[bestgps].lsats[n].num < 0);
                strcat(xbuf, cbuf);
                cbuf[0] = 0;
            }
            break;
        case 'T':
            sprintf(cbuf, ",Alt Date E-phv-dop Hdng Mode Pos Raw Status This Velo Ysats Ovvu Jsats");
            break;
        default:
            cbuf[0] = 0;
            break;
        }
        c++;
        strcat(xbuf, cbuf);
    }
    strcat(xbuf, "\r\n");
    //should be "\r\n" for network 
    n = strlen(xbuf);
    if (n != write(acpt[conn], xbuf, n)) {
        close(acpt[conn]);
        acpt[conn] = -1;
    }
}

static void doraw(char *str)
{
    int i;
    for (i = 0; i < amax; i++) {
        if (acpt[i] == -1)
            continue;
        if (!raw[i])
            continue;
        if (strlen(str) != write(acpt[i], str, strlen(str))) {
            close(acpt[i]);
            acpt[i] = -1;
        }
    }
}

//extern void dongjson(void);

static void dowatch()
{
    int i;
    char oy[4] = "oy";
    findbestgps();
    for (i = 0; i < amax; i++) {
        if (acpt[i] == -1)
            continue;
        if (watch[i] == 1)
            prtgpsinfo(i, oy);
    }
    //    dongjson();
    int n = strlen(xbuf);
    for (i = 0; i < amax; i++) {
        if (acpt[i] == -1)
            continue;
        if (watch[i] == 2) {
            if (n != write(acpt[i], xbuf, n)) {
                close(acpt[i]);
                acpt[i] = -1;
            }
        }
    }
}

static void teardown(int signo)
{
    fprintf(errfd, "Shutdown\n");
    signal(SIGCHLD, SIG_IGN);
    fflush(errfd);
    gpst[bestgps].mn++;
    rotatekml();
    fclose(errfd);

    unlink(pidlockfile);
    exit(0);
}

char *rtname = "WebGPSD";

#include <resolv.h>
#include <netdb.h>
#include <arpa/inet.h>

static int pilock(void)
{
    int fd;
    int pid;
    char mypid[50];
    char pidstr[32];
    FILE *pidlockfp;

    pidlockfp = fopen(pidlockfile, "r");
    if (pidlockfp != NULL) {
        fgets(pidstr, 30, pidlockfp);
        pid = atoi(pidstr);
        fclose(pidlockfp);
        if (pid <= 0 || kill(pid, 0) == -1)     //stale?
            unlink(pidlockfile);
        else
            return 1;           // active
    }
    sprintf(mypid, "%d\n", getpid());
    fd = open(pidlockfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0)
        return 1;
    write(fd, mypid, strlen(mypid));
    close(fd);
    return 0;
}

static void dobindlstn(int *sock, int port)
{
    struct sockaddr_in sin;
    int n;
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    n = 1;
    setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int));
    if (0 > bind(*sock, (struct sockaddr *) &sin, sizeof(sin)))
        exit(-1);
}

static void primelocaltime()
{
    char sessdir[256];
    struct timeval tv;
    struct tm *tmpt;

    gettimeofday(&tv, NULL);
    tmpt = gmtime(&tv.tv_sec);
    gpst[bestgps].scth = 1;
    gpst[bestgps].sc = tmpt->tm_sec;
    gpst[bestgps].mn = tmpt->tm_min;
    gpst[bestgps].hr = tmpt->tm_hour;
    gpst[bestgps].dy = tmpt->tm_mday;
    gpst[bestgps].mo = 1 + tmpt->tm_mon;
    gpst[bestgps].yr = tmpt->tm_year % 100;

    sprintf(sessdir, "%02d%02d%02d%02d%02d%02d", gpst[bestgps].yr, gpst[bestgps].mo, gpst[bestgps].dy, gpst[bestgps].hr,
      gpst[bestgps].mn, gpst[bestgps].sc);

    // create timestamped directory
    mkdir(sessdir, 0777);
    chdir(sessdir);
}

int thisms = 0;
static int watchms = 0;
static int getms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    thisms = tv.tv_usec / 1000 + 1000 * (tv.tv_sec % 100);
    return thisms;
}

void doaccept(int lstsoc)
{
    int n, i, acptmp;
    struct sockaddr_in sin;
    unsigned int u = sizeof(sin);

    acptmp = accept(lstsoc, (struct sockaddr *) &sin, &u);
    for (n = 0; n < amax; n++)
        if (acpt[n] == -1)
            break;
    if (n < MAXCONN) {
        if (n >= amax)
            amax++;
        u = sizeof(sin);
        raw[n] = watch[n] = 0;
        acpt[n] = acptmp;
        i = fcntl(acpt[n], F_GETFL, 0);
        fcntl(acpt[n], F_SETFL, i | O_NONBLOCK);
        fprintf(errfd, "acpt %d %d\n", n, acpt[n]);
        fflush(errfd);
    }
    else
        close(acptmp);
}

static void kmlanno(char *buf)
{
    int i, n = strlen(buf);
    getms();                    // mark time
    while (n && buf[n - 1] < ' ')       // strip trailing cr/lfs
        buf[--n] = 0;
    if (!n)
        return;
    for (i = 0; i < n; i++)     // clean nonprintable
        if (buf[i] < ' ' || buf[i] > 0x7e)
            buf[i] = '.';

    sprintf(xbuf, "<!--%05d%s -->\n", thisms, buf);
    strcat(buf, "\r\n");
    add2kml(xbuf);
}

#include <sys/wait.h>
static void reapkid(int signo)
{
    int status = 0, child = 9999;

    while (0 < (child = waitpid(-1, &status, WNOHANG))) {
        fprintf(errfd, "Child %d exited with status %d\n", child, status);
        // mostly kml zips, but if we add connect execs and they need retrys...
    }                           // end when no more child processes exit
    signal(SIGCHLD, reapkid);
}

void usage(char *errstr) {
    printf( "%s\nUsage:\nwebgpsd -X param\n"
	    "-k path-to-pid-lockfile\n"
	    "-l path-to-log-directory\n"
	    "-i MINUTES (5) for kml split interval\n"
	    "-p PORT (2947)\n"
	    "-r (allow run as root)\n"
	    "-h print this and exit\n", errstr);
    exit(1);
}

//static int lasmn = -1;
int main(int argc, char *argv[])
{
    int n, i;
    int lstn = -1, lmax = 0;
    fd_set fds, fds2, fdserr, lfds;
    struct sockaddr_in sin;
    char buf[512];
    //    struct timeval tv;
    unsigned int mainlock = 0;
    unsigned char rover = 1; // no root run;
    for( n = 1; n < argc; n++ ) {
	if( argv[n][0] != '-' )
	    usage("invalid parameter format");
	switch( argv[n][1] ) {
	case 'k':
	    pidlockfile = argv[++n];
	    break;
	case 'l':
	    logdirprefix = argv[++n];
	    break;
	case 'h':
	    usage("");
	    break;
	case 'i':
	    kmlinterval = atoi(argv[++n]);
	    if( kmlinterval <= 0 )
		usage("invalid kml split interval");
	    break;
	case 'p':
	    gpsdport = atoi(argv[++n]);
	    if( gpsdport <= 0 || gpsdport > 65535 )
		usage("invalid port");
	    break;
	case 'r':
	    rover = 0;
	    break;
	default:
	    usage("invalid parameter");
	    break;
	}
    }

    if (rover && !geteuid()) {
        fprintf(stderr, "Don't run as root!\n");
        exit(-2);
    }

    if (pilock())
        exit(-1);

    memset(&gpst, 0, sizeof(gpst));

    errfd = fopen("/tmp/gpsd.log", "w");
    fprintf(errfd, "Startup\n");
    fflush(errfd);

    // get config here
    chdir( logdirprefix );

    for (n = 0; n < MAXCONN; n++)
        acpt[n] = -1;

    for (i = 0; i < MAXSRC; i++)
        gpst[i].gpsfd = -2;

    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    dobindlstn(&lstn, gpsdport);

    primelocaltime();

    xbuf = malloc(BUFLEN);

    //place for OBD data before time and GPS gpst[bestgps].lock for position
    if (kmlinterval)
        logfd = fopen("prlock.kml", "a+b");
    add2kml("<Document><name>Pre-Lock</name><Placemark><LineString><coordinates>0,0,0\n");

    signal(SIGTERM, teardown);
    signal(SIGINT, teardown);
    signal(SIGQUIT, teardown);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, reapkid);

    listen(lstn, 3);

    lmax = lstn;
    FD_ZERO(&lfds);
    FD_SET(lstn, &lfds);

    fprintf(errfd, "listen g=%d\n", lstn);

    for (;;) {                  // main server loop
        fds = lfds;
        n = lmax;
#define MAXFD(fd) if (fd >= 0) { FD_SET(fd, &fds); if (fd > n) n = fd; }
        for (i = 0; i < amax; i++)
            MAXFD(acpt[i]);

        fds2 = fds;
        fdserr = fds;
        i = select(++n, &fds, NULL, &fdserr, NULL);
        if (i < 1)              // no activity - signal can break
            continue;

        /* accept new command connections */
        if (FD_ISSET(lstn, &fds))
            doaccept(lstn);

        /* read commands from connections  */
        for (i = 0; i < amax; i++) {
            if (acpt[i] == -1)
                continue;
            if (FD_ISSET(acpt[i], &fdserr)) {
                close(acpt[i]);
                acpt[i] = -1;
                continue;
            }
            if (FD_ISSET(acpt[i], &fds)) {
                n = read(acpt[i], buf, 511);
                if (n <= 0) {
                    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                        continue;
                    /* read error */
                    close(acpt[i]);
                    acpt[i] = -1;
                    continue;
                }
                buf[n] = 0;
                if (strstr(buf, "HTTP/1")) {    // http request
                    if (!strncmp(buf, "GET ", 4)) {     // only one allowed
                        strncpy(xbuf, buf, 128);
                        xbuf[128] = 0;  // force null term string
                        dowebget();
                        write(acpt[i], xbuf, strlen(xbuf));
                    }
                    close(acpt[i]);
                    acpt[i] = -1;
                    continue;
                }
                if (!strncmp(buf, ":GPS", 4)) {
                    kmlanno(buf);
                    char *b = &buf[1];
                    b = strchr(b, ':'); // find second colon
                    if (b) {
                        b++;
                        b = strchr(b, ':');     // find third colon
                        if (b)
                            b++;
                    }
                    if (!b)
                        continue;
                    doraw(b);
                    if (mainlock)
                        mainlock--;
                    i = getgpsinfo(acpt[i], buf, thisms);
                    // fresh, good data plus lock, reset aux counter
                    if (gpst[bestgps].gpsfd == acpt[i] && i > 0 && gpst[bestgps].lock)  ///////////////////////////////////////////////////
                        mainlock = 100;
                    continue;
                }
                if (!strncmp(buf, ":ANO", 4)) {
                    kmlanno(buf);
                    continue;
                }
#ifdef HARLEY
                if (!strncmp(buf, ":HOG", 4)) {
extern void calchog(char *outb, int mstime);
                    getms();
		    char *c = strchr( buf, 'J' );
                    calchog(c, thisms);
                    kmlanno(buf);
                    continue;
                }
#endif
                // finally, do command
                prtgpsinfo(i, buf);
            }
        }
        if ((100000 + thisms - watchms) % 100000 > 1000) {
            dowatch();
            watchms = thisms;
        }

    }
    return 0;                   // quiet compiler
}
