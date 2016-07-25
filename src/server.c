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




static int spectrum_lua_init(struct spectrum *sp)
{


    sp->L = luaL_newstate();
    luaL_openlibs(sp->L);

    lua_createtable(sp->L, 0 /* narr */, 116 /* nrec */);    /* sp.* */

    lua_createtable(sp->L, 0, 2 /* nrec */); /* metatable for .var */


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
        sp->threads[i].ovector_n = (sp->fields_n + 3) * 3;
        sp->threads[i].ovector = Malloc(sizeof(int) * sp->threads[i].ovector_n);
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

    debug("create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        spt->L = splua_init(sp, spt);
        if (!spt->L)
            return -1;
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
    sp->time = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
        time_end.tv_usec - time_start.tv_usec;

    //printf("TimeSpace: %lds %ldms %ldmi\n", t / 1000000,
    //        t % 1000000 / 1000,
    //        t % 1000
    //        );
    return 0;
}

static iterm_t *spectrum_iterm_get(struct sp_thread *spt, string_t *s)
{
    iterm_t *iterm;
    iterm = spt->summary_head;
    while (iterm)
    {
        if (s->l == iterm->name->l && !strncmp(iterm->name->s, s->s, s->l))
            return iterm;
        iterm = iterm->next;
    }
    return NULL;

}


static void spectrum_recod_iter(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;
    iterm_t *iterm1;
    iterm_t *iterm2;

    debug("create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        spt->L = splua_init(sp, spt);
        if (!spt->L)
            return;
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

    if (sp->thread_num > 1)
    {
        for (i=1; i < sp->thread_num; ++i)
        {
            iterm1 = sp->threads[i].summary_head;
            while (iterm1)
            {
                iterm2 = spectrum_iterm_get(sp->threads, iterm1->name);
                if (iterm2)
                    iterm2->v.n.n += iterm1->v.n.n;
                else{
                    sp->threads[0].summary_tail->next = iterm1;
                    sp->threads[0].summary_tail = iterm1;
                }

                iterm1 = iterm1->next;
            }
        }
    }
    iterm1 = sp->threads[0].summary_head;
    while(iterm1)
    {
        printf("%.*s = %f\n", (int)iterm1->name->l,
                iterm1->name->s, iterm1->v.n.n);
        iterm1 = iterm1->next;

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

    if (!sp->option_server_cycle)
        return 0;

    sp->server_fd = sws_net_server(sp->option_server_host,
            sp->option_server_port, false, 5);

    if (sp->server_fd < 0)
    {
        logerr("%s\n", geterr());
        return -1;
    }

    loginfo("Start server at %s:%d\n", sp->option_server_host,
            sp->option_server_port);

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
    if (!sp->file_log || !sp->file_pattern)
    {
        printf("should set file_log and file_pattern in spectrum_config "
                "of spectrum.lua\n");
        return -1;
    }

    debug("Pattern File: %s\n", sp->file_pattern);
    debug("Log File: %s\n", sp->file_log);

    if (pattern_compile(sp, sp->file_pattern))
    {
        printf("pattern_compile fail\n");
        return -1;
    }

    if (0 != spectrum_recod_reads(sp)) return -1;

    return spectrum_server_cycle(sp);
}


