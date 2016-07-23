/**
 *   author       :   丁雪峰
 *   time         :   2016-07-22 11:01:46
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>

#include "spectrum.h"

int main()
{
    struct spectrum *sp;
    struct sws_filebuf *log_buf;
    struct record *record;
    struct timeval time_start, time_end;

    gettimeofday(&time_start, NULL);

    log_buf = sws_fileread("t/ngx_logs");

    sp = compile("t/pattern");
    if (!sp)
    {
        printf("print sp compile fail\n");
        return -1;
    }

    sp->L = luaL_newstate();

    lua_createtable(sp->L, 0 /* narr */, 116 /* nrec */);    /* sp.* */
    //lua_setfield(L, -2, "sp"); /* ngx package loaded */
    record_lua_init(sp->L);

    lua_setglobal(sp->L, "sp");


    luaL_openlibs(sp->L);
    luaL_dofile(sp->L, "spectrum.lua");

    record_reads(sp, log_buf->buf, log_buf->size);
    record_iter(sp);



    gettimeofday(&time_end, NULL);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("TimeSpace: %lds %ldms %ldmi\n", time_end.tv_sec - time_start.tv_sec,
            (time_end.tv_usec - time_start.tv_usec)/1000,
            (time_end.tv_usec - time_start.tv_usec)%1000
            );



}
