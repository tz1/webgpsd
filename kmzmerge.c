// merge my special kmz compressed files in a directory into a single master kmz

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

unsigned char xbuf[1024 * 1024];
int main(int argc, char *argv[])
{
    int argp;

    if (argc == 1) {
        printf("Merges kmz files in a webgpsd generated directory into one kmz with all kml files\n");
        printf("usage: kmzmerge <webgpsd-kmzdir1> ...\n");
        return 1;
    }

    for (argp = 1; argp < argc; argp++) {

        DIR *kmzdir = opendir(argv[argp]);
        if (!kmzdir) {
            printf("%s is not a dir\n", argv[argp]);
            continue;
        }
        char mergn[80];
        char cdrtmp[80];
        while (argv[argp][strlen(argv[argp]) - 1] == '/')
            argv[argp][strlen(argv[argp]) - 1] = 0;
        strcpy(mergn, argv[argp]);
        strcat(mergn, ".kmz");
        int kmzout = open(mergn, O_RDWR | O_CREAT, 0644);       // WRONLY?
        if (kmzout < 0)
            return -3;
        strcpy(cdrtmp, mergn);
        strcat(cdrtmp, ".cendir");
        int kmzcdr = open(cdrtmp, O_RDWR | O_CREAT, 0644);      // WRONLY?
        if (kmzcdr < 0)
            return -4;

        struct dirent *onekmz;
        unsigned long totofst = 0;
        unsigned long totcsiz = 0;
        int filecnt = 0;
        unsigned char eocd[22];
        printf("merging: %s\n", argv[argp]);
        while ((onekmz = readdir(kmzdir))) {
            if (!strstr(onekmz->d_name, ".kmz"))
                continue;
            if (onekmz->d_name[1] < '0' || onekmz->d_name[1] > '9')
                continue;

            strcpy(mergn, argv[argp]);
            strcat(mergn, "/");
            strcat(mergn, onekmz->d_name);
            int onefd = open(mergn, O_RDONLY);
            if (onefd < 0)
                return -2;
            printf(" + %s\n", onekmz->d_name);
            read(onefd, xbuf, 30);
            unsigned long fnl = xbuf[26] + (xbuf[27] << 8);
            unsigned long csz = xbuf[18] + (xbuf[19] << 8) + (xbuf[20] << 16) + (xbuf[21] << 24);

            //        printf("%s, %s: %d %d\n", onekmz->d_name, mergn, fnl, csz);

            //FIXME - do up to sizeof(xbuf) and repeat as needed
            int vd = read(onefd, xbuf + 30, fnl + csz);
            if (vd != fnl + csz)
                return -4;
            vd = write(kmzout, xbuf, 30 + fnl + csz);
            if (vd != 30 + fnl + csz)
                return -5;

            int tot = read(onefd, xbuf, 1024);
            if (tot < 46 + fnl)
                return -6;
            close(onefd);
            filecnt++;
            totcsiz += 46 + fnl;

            xbuf[42] = totofst;
            xbuf[43] = totofst >> 8;
            xbuf[44] = totofst >> 16;
            xbuf[45] = totofst >> 24;

            totofst += 30 + fnl + csz;

            write(kmzcdr, xbuf, 46 + fnl);
            memcpy(eocd, xbuf + fnl + 46, 22);
        }
        eocd[8] = filecnt;
        eocd[9] = filecnt >> 8;
        eocd[10] = filecnt;
        eocd[11] = filecnt >> 8;
        eocd[12] = totcsiz;
        eocd[13] = totcsiz >> 8;
        eocd[14] = totcsiz >> 16;
        eocd[15] = totcsiz >> 24;
        eocd[16] = totofst;
        eocd[17] = totofst >> 8;
        eocd[18] = totofst >> 16;
        eocd[19] = totofst >> 24;

        lseek(kmzcdr, 0, SEEK_SET);
        int vd = read(kmzcdr, xbuf, 65536);
        if (vd < 0)
            return -8;
        write(kmzout, xbuf, vd);
        write(kmzout, eocd, 22);
        close(kmzout);
        close(kmzcdr);
        unlink(cdrtmp);
        closedir(kmzdir);
    }
    return 0;
}
