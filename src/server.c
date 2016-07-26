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


static int spectrum_log_alloc(struct spectrum *sp, uint64_t uint64_total)
{
    struct {
        const char *pos;
        uint64_t remain;
        uint64_t slice;
        iterm_t *file;
    } spi;

    struct {
        struct sp_thread *spt;

        uint64_t remain;
        iterm_t *log;
        iterm_t **next;
    } spti;
    int i;

    spi.slice = uint64_total / sp->thread_num;

    spi.file = sp->file_logs;
    spi.remain = spi.file->v.s.l;
    spi.pos = spi.file->v.s.s;


    for (i=0; i < sp->thread_num; ++i)
    {
        spti.spt = sp->threads + i;

        spti.remain = spi.slice;
        spti.next = &spti.spt->logs;


        while (spti.remain > 0)
        {
            if (spi.remain <= 0)
            {
                spi.file = spi.file->next;
                if (!spi.file)
                    return 0;
                spi.remain = spi.file->v.s.l;
                spi.pos = spi.file->v.s.s;
            }
            spti.log = *spti.next = Malloc(sizeof(iterm_t));
            spti.next = &spti.log->next;
            spti.log->next = NULL;


            if (spi.remain - spti.remain > 0)
            {
                spti.remain = 0;
                spti.log->v.s.s = (char *)spi.pos;
                spi.pos += spti.remain;

                while ('\n' != *spi.pos && spi.pos - spi.file->v.s.s < spi.file->v.s.l)
                    ++spi.pos;

                ++spi.pos;
                spti.log->v.s.l = spi.pos - spti.log->v.s.s;
                spi.remain = spi.file->v.s.l - (spi.pos - spi.file->v.s.s);

                continue;
            }

            spti.remain -= spi.remain;
            spti.log->v.s.s = (char *)spi.pos;
            spti.log->v.s.l = spi.remain;

            spi.remain = 0;
        }
    }
    return 0;
}

static uint64_t spectrum_open_log(struct spectrum *sp)
{
    iterm_t *iterm;
    int fd;
    struct stat st;
    uint64_t total_size = 0;

    iterm = sp->file_logs;
    while(iterm)
    {
        loginfo("open log file: %s\n", iterm->name->s);
        fd = open(iterm->name->s, O_RDONLY);
        if (fd < -1)
        {
            logerr("open file fial: %s\n", iterm->name->s);
            return 0;
        }
        if(-1 == fstat(fd, &st))
        {
            logerr("stat: %s", strerror(errno));
            return 0;
        }

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
    uint64_t total;
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

    debug("spectrum_pthread_create: create %d threads\n", sp->thread_num);
    for (i=0; i < sp->thread_num; ++i)
    {
        spt = sp->threads + i;

        if (!spt->L)
            spt->L = splua_init(sp, spt);

        if (!spt->L) return -1;

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
        if (s->l == iterm->name->l && !strncmp(iterm->name->s, s->s, s->l))
            return iterm;
        iterm = iterm->next;
    }
    return NULL;

}


static int spectrum_recod_reads(struct spectrum *sp)
{
    struct timeval time_start, time_end;

    gettimeofday(&time_start, NULL);

    if (spectrum_log_split(sp)) return -1;

    spectrum_pthread_create(sp, record_reads);

    spectrum_join(sp);

    gettimeofday(&time_end, NULL);
    sp->time = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
        time_end.tv_usec - time_start.tv_usec;

    return 0;
}


static void spectrum_recod_iter(struct spectrum *sp)
{
    int i;
    iterm_t *iterm1;
    iterm_t *iterm2;

    spectrum_pthread_create(sp, record_iter);

    spectrum_join(sp);


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
    if (!sp->file_pattern)
    {
        printf("should set file_log and file_pattern in spectrum_config "
                "of spectrum.lua\n");
        return -1;
    }

    debug("Pattern File: %s\n", sp->file_pattern);

    if (pattern_compile(sp, sp->file_pattern))
    {
        printf("pattern_compile fail\n");
        return -1;
    }

    if (0 != spectrum_recod_reads(sp)) return -1;

    return spectrum_server_cycle(sp);
}


