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

static struct spectrum *spectrum_init()
{
    struct spectrum *sp;
    int i;
    sp = Malloc(sizeof *sp);
    memset(sp, 0, sizeof *sp);

    sp->thread_num = sysconf(_SC_NPROCESSORS_ONLN);
    sp->threads = Malloc(sizeof(struct sp_thread) * sp->thread_num);
    memset(sp->threads, 0, sizeof(struct sp_thread) * sp->thread_num);

    for (i=0; i < sp->thread_num; ++i)
    {
        sp->threads[i].sp = sp;
    }

    return sp;
}


static void spectrum_recod_reads(struct spectrum *sp)
{
    int i;
    struct sp_thread *spt;
    struct sws_filebuf *log_buf;
    unsigned short thread_num;
    thread_num = sp->thread_num;

    // thread_num
    log_buf = sws_fileread("/home/vagrant/tmp/test.log");
    thread_num = MIN(log_buf->size/ 1024/ 1024, thread_num);
    thread_num = MAX(1, thread_num);

    {
        int size;
        const char *log;
        size = log_buf->size / thread_num;
        log = log_buf->buf;

        // split log
        for (i=0; i < thread_num; ++i)
        {
            spt = sp->threads + i;
            spt->log = log;

            if (1 == thread_num - i) // last one
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

    printf("create %d threads\n", thread_num);
    for (i=0; i < thread_num; ++i)
    {
        spt = sp->threads + i;
        record_lua_init(spt);
        if (0 != pthread_create(&spt->tid, 0, record_reads, spt))
        {
            printf("create thread `%d' fail\n", i);
        }
    }

    for (i=0; i < thread_num; ++i)
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

    if (0 != pattern_compile(sp, "t/pattern"))
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
    printf("TimeSpace: %lds %ldms %ldmi\n", time_end.tv_sec - time_start.tv_sec,
            (time_end.tv_usec - time_start.tv_usec)/1000,
            (time_end.tv_usec - time_start.tv_usec)%1000
            );
}
