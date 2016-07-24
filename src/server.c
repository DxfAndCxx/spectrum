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
#include <sys/wait.h>

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


static int spectrum_lua_init(struct spectrum *sp)
{

    if (0 != access(sp->file_rc, R_OK))
    {
        printf("cannot access `%s'\n", sp->file_rc);
        return -1;
    }

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

    return 0;
}


static int spectrum_log_split(struct spectrum *sp)
{
    // thread_num
    struct sws_filebuf *log_buf;
    int size, i;
    const char *log;
    struct sp_thread *spt;

    // calc thread_num
    log_buf = sws_fileread(sp->file_log);
    if (!log_buf)
    {
        logerr("open `%s' fail\n", sp->file_log);
        return -1;
    }

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
    return 0;
}


static int spectrum_recod_reads(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;
    struct timeval time_start, time_end;
    gettimeofday(&time_start, NULL);

    if (0 != spectrum_log_split(sp))
    {
        return -1;
    }

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
   //         printf("thread %d finish\n", i);
            spt->tid = 0;
        }
    }

    gettimeofday(&time_end, NULL);
    int64_t t;
    t = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
        time_end.tv_usec - time_start.tv_usec;

    printf("TimeSpace: %lds %ldms %ldmi\n", t / 1000000,
            t % 1000000 / 1000,
            t % 1000
            );
    return 0;
}



static void spectrum_recod_iter(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;

    printf("create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        record_lua_init(spt);
        if (0 != pthread_create(&spt->tid, 0, record_iter, spt))
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
//            printf("thread %d finish\n", i);
            spt->tid = 0;
        }
    }
}

typedef void (*spectrum_cmd_handle)(struct spectrum *sp);

static void spectrum_fork(struct spectrum *sp, spectrum_cmd_handle handle)
{
    pid_t id;
    id = fork();
    if (0 == id)
    {
        // close cycle
        sp->option_server_cycle = 0;

        // dup2 fd
        if (!dup2(sp->confd, 1))
        {
            logerr("dup2 confd[%d] to 1 fail\n", sp->confd);
            return;
        }
        if (!dup2(sp->confd, 2))
        {
            logerr("dup2 confd[%d] to 2 fail\n", sp->confd);
            return;
        }

        handle(sp);

        close(1);
        close(2);
    }
    else{
        waitpid(id, 0, 0);
    }
}


static void spectrum_handle_cmd(struct spectrum *sp, const char *cmd)
{
    if (!strncmp("iter\r\n", cmd, 6))
    {
        spectrum_fork(sp, spectrum_recod_iter);
        return;
    }

    if (!strncmp("stop\r\n", cmd, 6))
    {
        sp->option_server_cycle = 0;
        return;
    }

    const char *msg;
    msg = "can not distinguish the command\n";

    send(sp->confd, msg, strlen(msg), 0);
}


static int spectrum_server_cycle(struct spectrum *sp)
{
    char buf[30];

    if (listen(sp->server_fd, 5))
    {
        logerr("%s\n", strerror(errno));
        return -1;
    }

    while (sp->option_server_cycle)
    {
        sp->confd = accept(sp->server_fd, NULL, 0);

        read(sp->confd, buf, sizeof(buf));
        spectrum_handle_cmd(sp, buf);
        close(sp->confd);
    }

    close(sp->server_fd);
    return 0;
}


int spectrum_start_server(struct spectrum *sp)
{

    sp->server_fd = sws_net_bind(sp->option_server_host,
            sp->option_server_port, 0);

    if (sp->server_fd < 0)
    {
        logerr("%s\n", geterr());
        return -1;
    }

    if (0 != spectrum_lua_init(sp)) return -1;

    sp_stage_lua_call(sp->L, "spectrum_config");

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

    if (0 != spectrum_recod_reads(sp)) return -1;

    sp->option_server_cycle = 1;

    return spectrum_server_cycle(sp);
}


