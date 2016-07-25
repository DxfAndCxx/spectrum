/**
 *   author       :   丁雪峰
 *   time         :   2016-07-24 04:08:22
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "spectrum.h"

static struct spectrum *spectrum_init()
{
    struct spectrum *sp;

    sp = Malloc(sizeof *sp);
    memset(sp, 0, sizeof *sp);

    sp->thread_num = sysconf(_SC_NPROCESSORS_ONLN);
    sp->option_server_port = 8991;
    sp->option_server_host = "127.0.0.1";
    sp->option_server_cycle = 1;


    sp->file_rc = "spectrum.lua";
    return sp;
}


static int spectrum_options(struct spectrum *sp, int argc, const char **argv)
{
    int i;
    const char *p;

    i = 1;
    while(i < argc)
    {
        p = argv[i++];

        if ('-' != *p++)
        {
            printf("invalid option: `%s'\n", p - 1);
            return -1;
        }

        switch(*p)
        {
            case 's':
                sp->option_server_cycle = 0;

            case 'S':
                sp->option_work_as_server = 1;
                break;

            case 'f':
                if (i == argc)
                {
                    printf("option: `%s' except arg\n", p - 1);
                    return -1;
                }
                sp->file_rc = argv[i++];
                break;

            case 'c':
                if (i == argc)
                {
                    printf("option: `%s' except arg\n", p - 1);
                    return -1;
                }
                sp->option_client_cmd = argv[i++];
                break;

            default:
                printf("invalid option: `%s'\n", p - 1);
                return -1;
        }
    }

    return 0;
}


int main(int argc, const char **argv)
{
    struct spectrum *sp;

    // start
    sp = spectrum_init();
    if (!sp)
    {
        printf("print sp init fail\n");
        return -1;
    }

    if (0 != spectrum_options(sp, argc, argv)) return -1;

    if (sp->option_work_as_server)
    {
        return spectrum_start_server(sp);
    }
    return spectrum_start_client(sp);
}
