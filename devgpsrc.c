#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/termios.h>

int main(int argc, char *argv[] ) {
    int wgsock, serfd = 0;
    int port = 2947;
    struct sockaddr_in sin;
    int i;
    unsigned int ll;
    struct timeval tv;
    char xbuf[256];
    unsigned char buf[256];
    char nodeid[5];

    memset((char *) &sin, 0, sizeof(sin));
    ll = inet_addr("127.0.0.1");
    memcpy(&sin.sin_addr, &ll, sizeof(ll));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    wgsock = socket(AF_INET, SOCK_STREAM, 0);
    i = connect(wgsock, (struct sockaddr *) &sin, sizeof(sin));
    if( i )
	exit(i);

    if( argc > 1 ) {
	strcpy( nodeid, &argv[1][strlen(argv[1])-4]);
	serfd = open( argv[1], O_RDWR );
	if( serfd < 0 )
	    exit(serfd);

{
    struct termios termst;
    if (-1 == tcgetattr(serfd, &termst))
        return -1;

    cfmakeraw(&termst);

    termst.c_iflag |= IGNCR;
    termst.c_lflag |= ICANON;

    tcsetattr(serfd, TCSANOW, &termst);
    tcflush(serfd, TCIOFLUSH);
}



    }
    else {
	sprintf( nodeid, "%04d", getpid() );
    }
    gettimeofday(&tv, NULL);
    if( argc > 1 )
	sprintf( xbuf, ":ANOD%s:%03ld:%s\n", nodeid, tv.tv_usec / 1000, argv[1] );
    else
	sprintf( xbuf, ":ANOD%s:%03ld:(stdin)\n", nodeid, tv.tv_usec / 1000 );
    write(wgsock, xbuf, strlen(xbuf));
    for(;;) {
	i = read(serfd, buf, sizeof(buf)-1);
	if( i <= 0 )
	    break;
	buf[i]=0;
	while( i && buf[i] <= ' ' )
	    buf[i--]=0;
	gettimeofday(&tv, NULL);
	sprintf( xbuf, ":GPSD%s:%03ld:%s\n", nodeid, tv.tv_usec / 1000, buf );
	write(wgsock, xbuf, strlen(xbuf));
	//	printf("%s",xbuf);
    }
    return i;

}
