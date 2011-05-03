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
    int port = 2947;
    struct sockaddr_in sin;
    int i;
    unsigned int ll;
    struct timeval tv;
    char xbuf[256];

    memset((char *) &sin, 0, sizeof(sin));
    ll = inet_addr("127.0.0.1");
    memcpy(&sin.sin_addr, &ll, sizeof(ll));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    wgsock = socket(AF_INET, SOCK_STREAM, 0);
    i = connect(wgsock, (struct sockaddr *) &sin, sizeof(sin));
    if( i )
	exit(i);

    if ((i2cfd = open("/dev/i2c-0", O_RDWR)) < 0)
        return 1;
    if (ioctl(i2cfd, I2C_SLAVE, 0x54) < 0)
        return 2;

    unsigned char iobuf[24];
    gettimeofday(&tv, NULL);
    sprintf( xbuf, ":ANODJDATA:%03ld:(stdin)\n", tv.tv_usec / 100 );
    write(wgsock, xbuf, strlen(xbuf));

    for (;;) {
        memset(iobuf, 0, sizeof(iobuf));
	read(i2cfd, iobuf, 20);
	int i; 
	for (i = 0; i < 20; i++)
	    if (iobuf[i] >= 127)
		break; 
	iobuf[i] = 0;
	if( !strlen((char *)iobuf) )
	    continue;
	gettimeofday(&tv, NULL);
	sprintf( xbuf, ":HOGDJDATA:%03ld:%s\n", tv.tv_usec / 100, iobuf );
	write(wgsock, xbuf, strlen(xbuf));
    }
    close(i2cfd);
    close(wgsock);
    return 0;
}
