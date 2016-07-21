/**
 *   author       :   丁雪峰
 *   time         :   2016-07-21 07:06:01
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

char *sws_fileread(const char *path)
{
    struct stat st;

    int f = open(path, O_RDONLY);
    if (-1 == f){
        seterr("open: %s", strerror(errno));
        return -1;
    }

    if(-1 == fstat(f, &st))
    {
        seterr("stat: %s", strerror(errno));
        return -1;
    }

    *buf = malloc(st.st_size + 2);
    *size = read(f, *buf, st.st_size);

    (*buf)[*size] = 0;
    close(f);
    return 0;
}
