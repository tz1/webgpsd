#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    int wgsock, i2cfd;
    int i;
    struct timeval tv;
    char xbuf[256];


    if ((i2cfd = open("/dev/i2c-0", O_RDWR)) < 0)
        return 1;
    if (ioctl(i2cfd, I2C_SLAVE, 0x54) < 0)
        return 2;

    int port = 2947;
    struct sockaddr_in sin;
    unsigned int ll;
    memset((char *) &sin, 0, sizeof(sin));
    ll = inet_addr("127.0.0.1");
    memcpy(&sin.sin_addr, &ll, sizeof(ll));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    wgsock = socket(AF_INET, SOCK_STREAM, 0);
    i = connect(wgsock, (struct sockaddr *) &sin, sizeof(sin));
    if( i )
	exit(i);

    const unsigned char iosiz=20;
    char i2cbuf[iosiz], outbuf[iosiz*2];
    gettimeofday(&tv, NULL);
    sprintf( xbuf, ":ANODJDATA:%03ld:(stdin)\n", tv.tv_usec / 1000 );
    write(wgsock, xbuf, strlen(xbuf));
    outbuf[0] = 0;
    for (;;) {
	i2cbuf[iosiz-2] = 0;
	int i; 
	i = read(i2cfd, i2cbuf, iosiz-2);
	if( i <= 0 )
	    continue;
	for (i = 0; i < iosiz-2; i++)
	    if (i2cbuf[i] >= 127)
		break;
	i2cbuf[i] = 0;
	strcat(outbuf, i2cbuf);
	char *c;
	while( (c = strchr( outbuf, '\n' ))) {
	    while( *c == '\n' )
		*c++ = 0;
	    if( strlen(outbuf) > 7  && strchr( outbuf, 'J') ) {
		gettimeofday(&tv, NULL);
		sprintf( xbuf, ":HOGDJDAT:%03ld:%s\n", tv.tv_usec / 1000, outbuf );
		write(wgsock, xbuf, strlen(xbuf));
	    }
	    if( !*c )
		outbuf[0] = 0;
	    else
		strcpy( outbuf, c);
	}
    }
    close(i2cfd);
    close(wgsock);
    return 0;
}
