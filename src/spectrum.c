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

#include "spectrum.h"

int main()
{
    struct spectrum *sp;
    struct sws_filebuf *log_buf;
    struct record *record;
    struct item_string *v;

    log_buf = sws_fileread("t/ngx_logs");

    sp = compile("t/pattern");
    if (!sp)
    {
        printf("print sp compile fail\n");
        return -1;
    }

    sp->L = luaL_newstate();

    luaL_openlibs(sp->L);
    luaL_dofile(sp->L, "spectrum.lua");

    record_reads(sp, log_buf->buf, log_buf->size);

    record = sp->record;


    while (record)
    {
        v = record->string;
        printf("==================================\n");
        while (v)
        {

            printf("%-15.*s: %.*s\n", v->name->l, v->name->s, v->s.l, v->s.s);
            v = v->next;
        }
        record = record->next;
    }
}
