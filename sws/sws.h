/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:04:55
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __SWS_H__
#define __SWS_H__

#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "file.h"
#include "net.h"
#include "str.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define SWS_AG -2
#define SWS_ERR -1
#define SWS_OK 0

#define timeval_diff(start, end) ((end.tv_sec - start.tv_sec) * 1000 +\
    (end.tv_usec - start.tv_usec)/1000)

extern char *errstr;
extern char errbuf[1024];

static inline void seterr(const char *fmt, ...)
{
    va_list argList;

    va_start(argList, fmt);
    vsnprintf(errbuf, sizeof(errbuf), fmt, argList);
    va_end(argList);

    errstr = errbuf;
}

static inline const char *geterr()
{
    return errstr;
}

#define SWS_AP_INT    (void *)1
#define SWS_AP_BOOL   (void *)2
#define SWS_AP_FLOAT  (void *)3
#define SWS_AP_DOUBLE (void *)4
#define SWS_AP_STRING (void *)5

void sws_ap_opt(const char *arg, void *value, void *type, const char *help);

#define sws_ap_bool(arg, value, help) sws_ap_opt(arg, value, SWS_AP_BOOL, help)
#define sws_ap_int(arg, value, help) sws_ap_opt(arg, value, SWS_AP_INT, help)
#define sws_ap_str(arg, value, help) sws_ap_opt(arg, value, SWS_AP_STRING, help)
#define sws_ap_double(arg, value, help) sws_ap_opt(arg, value, SWS_AP_DOUBLE, help)
#define sws_ap_func(arg, value, func, help) sws_ap_opt(arg, value, func, help)

int sws_ap(int argc, const char **argv);
#endif


