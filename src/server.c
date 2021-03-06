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
#include <sys/mman.h>


static int spectrum_log_alloc(struct spectrum *sp, int64_t int64_total)
{
    struct {
        const char *pos;
        int64_t remain;
        int64_t slice;
        iterm_t *file;
    } T;

    struct {
        struct sp_thread *spt;

        int64_t remain;
        iterm_t *log;
        iterm_t **next;
    } t;
    int i;

    T.slice = int64_total / sp->thread_num;

    T.file = sp->file_logs;
    T.remain = T.file->v.s.l;
    T.pos = T.file->v.s.s;


    for (i=0; i < sp->thread_num; ++i)
    {
        t.spt = sp->threads + i;

        t.remain = T.slice;
        t.next = &t.spt->logs;


        while (t.remain > 0)
        {
            if (T.remain <= 0)
            {
                T.file = T.file->next;
                if (!T.file)
                    return 0;
                T.remain = T.file->v.s.l;
                T.pos = T.file->v.s.s;
            }
            t.log = *t.next = Malloc(sizeof(iterm_t));
            t.next = &t.log->next;
            t.log->next = NULL;


            if (T.remain > t.remain)
            {
                t.log->name = T.file->name;
                t.log->v.s.s = (char *)T.pos;
                T.pos += t.remain;

                while (T.pos - T.file->v.s.s < T.file->v.s.l)
                {
                    if ('\n' == *T.pos) break;
                    ++T.pos;
                }

                if (T.pos - T.file->v.s.s < T.file->v.s.l - 1)
                    ++T.pos;

                t.log->v.s.l = T.pos - t.log->v.s.s;
                T.remain = T.file->v.s.l - (T.pos - T.file->v.s.s);

                t.remain = 0;
                continue;
            }

            t.log->name = T.file->name;
            t.remain -= T.remain;
            t.log->v.s.s = (char *)T.pos;
            t.log->v.s.l = T.remain;

            T.remain = 0;
        }
    }
    return 0;
}


static int64_t spectrum_open_log(struct spectrum *sp)
{
    iterm_t *iterm;
    int fd;
    struct stat st;
    int64_t total_size = 0;

    iterm = sp->file_logs;
    while(iterm)
    {
        fd = open(iterm->name.s, O_RDONLY);
        if (fd < 0)
        {
            logerr("open file fial: %s\n", iterm->name.s);
            return 0;
        }

        if(-1 == fstat(fd, &st))
        {
            logerr("stat: %s", strerror(errno));
            return 0;
        }
        loginfo("* Open log file: %s size: %d\n", iterm->name.s, st.st_size);

        total_size += st.st_size;
        iterm->v.s.s = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if ((void *)-1 == iterm->v.s.s)
        {
            logerr("%s\n", strerror(errno));
            return -1;
        }
        iterm->v.s.l = st.st_size;

        close(fd);
        iterm = iterm->next;
    }

    return total_size;
}

static int spectrum_log_split(struct spectrum *sp)
{
    // thread_num
    struct sp_thread *spt;
    int64_t total;
    int i;

    total = spectrum_open_log(sp);

    if (!total) return -1;

    sp->thread_num = MIN(total / sp->option_slice_size, sp->thread_num);
    sp->thread_num = MAX(1, sp->thread_num);

    sp->threads = Malloc(sizeof(struct sp_thread) * sp->thread_num);
    memset(sp->threads, 0, sizeof(struct sp_thread) * sp->thread_num);

    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;

        spt->sp = sp;
        spt->ovector_n = (sp->fields_n + 3) * 3;
        spt->ovector = Malloc(sizeof(int) * spt->ovector_n);
    }

    spectrum_log_alloc(sp, total);
    return 0;
}


static int spectrum_pthread_create(struct spectrum *sp, void *handle)
{
    int i;
    struct sp_thread *spt;

    loginfo("* Create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;

        if (spt->lua_env.L)
        {
            lua_settop(spt->lua_env.L, 0);
            lua_close(spt->lua_env.L);
        }

        if (splua_init(sp, spt, &spt->lua_env))
            return -1;

        if (!spt->lua_env.L) return -1;

        if (0 != pthread_create(&spt->tid, 0, handle, spt))
        {
            logerr("create thread `%d' fail\n", i);
        }
    }

    return 0;
}


static int spectrum_join(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;

    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        if (spt->tid)
        {
            pthread_join(spt->tid, NULL);
            spt->tid = 0;
        }
    }
    return 0;
}


static iterm_t *spectrum_iterm_get(struct sp_thread *spt, string_t *s)
{
    iterm_t *iterm;
    iterm = spt->summary_head;
    while (iterm)
    {
        if (s->l == iterm->name.l && !strncmp(iterm->name.s, s->s, s->l))
            return iterm;
        iterm = iterm->next;
    }
    return NULL;

}


static int spectrum_recod_reads(struct spectrum *sp)
{
    struct timeval time_start, time_end;
    int64_t nums[5] = {0};
    struct sp_thread *spt;
    int i;

    gettimeofday(&time_start, NULL);

    if (spectrum_log_split(sp)) return -1;

    spectrum_pthread_create(sp, record_reads);

    spectrum_join(sp);

    gettimeofday(&time_end, NULL);
    sp->time = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
        time_end.tv_usec - time_start.tv_usec;

    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;
        nums[0] += spt->records_num;
        nums[1] += spt->records_num_nomatch;
        nums[2] += spt->records_num_errmatch;
        nums[3] += spt->records_num_droped;
        nums[4] += spt->lines_num;
    }
    loginfo("* Lines: %lu Records: %lu NoMatch: %lu Err: %lu Drop: %lu\n",
            nums[4], nums[0], nums[1], nums[2], nums[3]);

    loginfo("* Time: %lu.%lu\n",
            sp->time/ 1000000, sp->time % 1000000);



    return 0;
}


static void spectrum_recod_iter(struct spectrum *sp)
{
    int i, nargs;
    struct sp_thread *spt;
    lua_env_t env;
    script_t **script;

    spectrum_pthread_create(sp, record_iter);

    spectrum_join(sp);

    splua_init(sp, NULL, &env);

    script = env.scripts_reduce;

    while(*script)
    {
        nargs = 0;
        lua_settop(env.L, 0);
        lua_getglobal(env.L, "scripts");
        lua_getfield(env.L, -1, (*script)->name);
        lua_getfield(env.L, 2, "reduce");

        for (i=0; i < sp->thread_num; ++i)
        {
            int level;
            spt = sp->threads + i;
            level = (*(spt->lua_env.scripts_reduce +
                    (script - env.scripts_reduce)))->level;

            if (!splua_copy_table(env.L, spt->lua_env.L, level))
                ++nargs;
        }

        splua_pcall(env.L, nargs, 0);

        ++script;
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
        if (sp->option_redirt_out)
        {
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
        }


        handle(sp);

        //close(1);
        //close(2);
    }
    else{
        waitpid(id, 0, 0);
    }
}


static void spectrum_handle_cmd(struct spectrum *sp, const char *cmd)
{
    const char *C;
    time_t now ;
    struct tm *tm_now ;
    const char *response = "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n";

    time(&now) ;
    tm_now = localtime(&now) ;


    C = "GET /iter HTTP/1.1\r\n";
    if (!strncmp(C, cmd, strlen(C)))
    {
        write(sp->confd, response, strlen(response));
        loginfo("* [%d-%d-%d %d:%d:%d] recv request: %.*s\n",
                tm_now->tm_year+1900,
                tm_now->tm_mon+1,
                tm_now->tm_mday,
                tm_now->tm_hour,
                tm_now->tm_min,
                tm_now->tm_sec,
                strlen(C) -2 , C);

        spectrum_fork(sp, spectrum_recod_iter);
        return;
    }


    C = "iter\r\n";
    if (!strncmp(C, cmd, strlen(C)))
    {
        loginfo("* [%d-%d-%d %d:%d:%d] recv request: %.*s\n",
                tm_now->tm_year+1900,
                tm_now->tm_mon+1,
                tm_now->tm_mday,
                tm_now->tm_hour,
                tm_now->tm_min,
                tm_now->tm_sec,
                strlen(C) -2 , C);
        spectrum_fork(sp, spectrum_recod_iter);
        return;
    }

    C = "stop\r\n";
    if (!strncmp(C, cmd, strlen(C)))
    {
        loginfo("* [%d-%d-%d %d:%d:%d] recv request: %.*s\n",
                tm_now->tm_year+1900,
                tm_now->tm_mon+1,
                tm_now->tm_mday,
                tm_now->tm_hour,
                tm_now->tm_min,
                tm_now->tm_sec,
                strlen(C) -2 , C);

        sp->option_server_cycle = 0;
        return;
    }

    const char *msg;
    msg = "can not distinguish the command\n";

    send(sp->confd, msg, strlen(msg), 0);
}


static int spectrum_server_cycle(struct spectrum *sp)
{
    char buf[1024 * 4];


    sp->server_fd = sws_net_server(sp->option_server_host,
            sp->option_server_port, false, 5);

    if (sp->server_fd < 0)
    {
        logerr("%s\n", geterr());
        return -1;
    }

    loginfo("* Start server at %s:%d\n", sp->option_server_host,
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
    if (!sp->file_pattern && !sp->option_src_type)
    {
        logerr("should set file_log and file_pattern in spectrum_config "
                "of spectrum.lua\n");
        return -1;
    }

    if (0 == sp->option_src_type)
    {
        debug("Pattern File: %s\n", sp->file_pattern);

        if (pattern_compile(sp, sp->file_pattern))
        {
            logerr("pattern_compile fail\n");
            return -1;
        }

    }

    if (0 != spectrum_recod_reads(sp)) return -1;
    if (sp->option_server_cycle)
        return spectrum_server_cycle(sp);

    spectrum_recod_iter(sp);
    return 0;
}


