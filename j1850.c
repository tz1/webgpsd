#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

int main(int argc, char **argv)
{
    int i2cfd;
    if ((i2cfd = open("/dev/i2c-0", O_RDWR)) < 0)
        return 1;
    if (ioctl(i2cfd, I2C_SLAVE, 0x54) < 0)
        return 2;


    const unsigned char iosiz=24;
    char i2cbuf[iosiz], outbuf[iosiz*2];
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
	    if( strlen(outbuf) ) {
		printf( "%s\n", outbuf );
		fflush(stdout);
	    }
	    if( !*c )
		outbuf[0] = 0;
	    else
		strcpy( outbuf, c);
	}
    }
    close(i2cfd);
    return 0;
}
