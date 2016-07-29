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
    sp->option_slice_size = 1024 * 1024 * 100;


    sp->file_rc = "spectrum.lua";
    return sp;
}


static int spectrum_args_append(void *value, const char *arg)
{
    iterm_t **iterm;
    string_t *name;

    iterm = (iterm_t **)value;
    while (*iterm)
        iterm = &(*iterm)->next;

    *iterm = Malloc(sizeof(iterm_t));
    name = Malloc(sizeof(*name));
    name->s = (char *)arg;
    name->l = strlen(name->s);
    (*iterm)->name = name;
    (*iterm)->next = NULL;

    return 0;
}


static int spectrum_options(struct spectrum *sp, int argc, const char **argv)
{
    const char *help;

    help = "work as server.";
    sws_argparser_add("-s", &sp->option_server_cycle, SWS_AP_BOOL, help);

    help = "set log file, can set multi times.";
    sws_argparser_add("-l", &sp->file_logs, spectrum_args_append, help);

    help = "set pattern file.";
    sws_argparser_add("-p", &sp->file_pattern, spectrum_args_append, help);

    help = "set rc.lua file. default is ./spectrum.lua.";
    sws_argparser_add("-r", &sp->file_rc, SWS_AP_STRING, help);

    help = "set the client cmd and work as client.";
    sws_argparser_add("-c", &sp->option_client_cmd, SWS_AP_STRING, help);

    return sws_argparser(argc, argv);
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

    sp->L = splua_init(sp, sp);
    if (!sp->L) return -1;

    sp_stage_lua_call(sp->L, "spectrum_config");

    if (sp->option_client_cmd)
    {
        return spectrum_start_client(sp);
    }
    return spectrum_start_server(sp);
}
