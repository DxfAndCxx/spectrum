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
#include <unistd.h>

#include "spectrum.h"

static int spectrum_lua_var_set(lua_State *L)
{
    struct spectrum *sp;
    string_t *field;
    string_t *value;

    lua_getglobal(L, "__sp");
    sp = lua_touserdata(L, -1);
    lua_pop(L, 1);

    field = sp_lua_tolstring(L, 2);
    value = sp_lua_tolstring(L, 3);

    if (0 == strncmp("file_log", field->s, field->l))
    {
        sp->file_log = value->s;
        debug("sp.file_log = %s\n", sp->file_log);
        return 0;
    }

    if (0 == strncmp("file_pattern", field->s, field->l))
    {
        sp->file_pattern = value->s;
        debug("sp.file_pattern = %s\n", sp->file_pattern);
        return 0;
    }
    return 0;
}


static void spectrum_lua_init(struct spectrum *sp)
{
    sp->L = luaL_newstate();
    luaL_openlibs(sp->L);

    lua_createtable(sp->L, 0 /* narr */, 116 /* nrec */);    /* sp.* */

    lua_createtable(sp->L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(sp->L, spectrum_lua_var_set);
    lua_setfield(sp->L, -2, "__newindex");

    lua_setmetatable(sp->L, -2);

    lua_setglobal(sp->L, "sp");

    if (0 != luaL_dofile(sp->L, sp->file_rc))
    {
        printf("dofile `%s' err: %s\n", sp->file_rc, lua_tostring(sp->L, -1));
        lua_pop(sp->L, 1);
    }

    lua_pushlightuserdata(sp->L, sp);
    lua_setglobal(sp->L, "__sp");

}


static struct spectrum *spectrum_init()
{
    struct spectrum *sp;

    sp = Malloc(sizeof *sp);
    memset(sp, 0, sizeof *sp);

    sp->thread_num = sysconf(_SC_NPROCESSORS_ONLN);


    sp->file_rc = "spectrum.lua";

    spectrum_lua_init(sp);

    sp_stage_lua_call(sp->L, "spectrum_config");
    return sp;
}

static void spectrum_log_split(struct spectrum *sp)
{
    // thread_num
    struct sws_filebuf *log_buf;
    int size, i;
    const char *log;
    struct sp_thread *spt;

    // calc thread_num
    log_buf = sws_fileread(sp->file_log);
    sp->log_buf = log_buf;

    sp->thread_num = MIN(log_buf->size/ 1024/ 1024, sp->thread_num);
    sp->thread_num = MAX(1, sp->thread_num);

    sp->threads = Malloc(sizeof(struct sp_thread) * sp->thread_num);
    memset(sp->threads, 0, sizeof(struct sp_thread) * sp->thread_num);

    for (i=0; i < sp->thread_num; ++i)
    {
        sp->threads[i].sp = sp;
    }

    // split log
    size = log_buf->size / sp->thread_num;
    log = log_buf->buf;

    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        spt->log = log;

        if (1 == sp->thread_num - i) // last one
        {
            spt->loglen = log_buf->buf + log_buf->size - spt->log;
        }
        else{
            log = log + size;
            while ('\n' != *log) ++log;
            ++log;

            spt->loglen = log - spt->log;
        }
    }

}


static void spectrum_recod_reads(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;

    spectrum_log_split(sp);

    printf("create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        record_lua_init(spt);
        if (0 != pthread_create(&spt->tid, 0, record_reads, spt))
        {
            printf("create thread `%d' fail\n", i);
        }
    }

    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        if (spt->tid)
        {
            pthread_join(spt->tid, NULL);
            printf("thread %d finish\n", i);
            spt->tid = 0;
        }
    }
}



int main()
{
    struct spectrum *sp;
    struct timeval time_start, time_end;

    gettimeofday(&time_start, NULL);

    // start

    sp = spectrum_init();
    if (!sp)
    {
        printf("print sp init fail\n");
        return -1;
    }

    if (!sp->file_log || !sp->file_pattern)
    {
        printf("should set file_log and file_pattern in spectrum_config "
                "of spectrum.lua\n");
        return -1;
    }


    if (0 != pattern_compile(sp, sp->file_pattern))
    {
        printf("pattern_compile fail\n");
        return -1;
    }

    spectrum_recod_reads(sp);

    // iter after read all records
    //record_iter(sp);

    // summary
//    sp_stage_lua_call(sp->L, "spectrum_summary");


    // end
    gettimeofday(&time_end, NULL);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    int64_t t;
    t = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
        time_end.tv_usec - time_start.tv_usec;

    printf("TimeSpace: %lds %ldms %ldmi\n", t / 1000000,
            t % 1000000 / 1000,
            t % 1000
            );
}
