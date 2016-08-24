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
    sp->option_log_level = LogLevelInfo;
    sp->option_redirt_out = 1;

    return sp;
}


static int spectrum_args_append(void *value, const char *arg)
{
    iterm_t **iterm;

    iterm = (iterm_t **)value;
    while (*iterm)
        iterm = &(*iterm)->next;

    *iterm = Malloc(sizeof(iterm_t));
    (*iterm)->name.s = (char *)arg;
    (*iterm)->name.l = strlen((char *)arg);
    (*iterm)->next = NULL;

    return 0;
}


static int spectrum_options(struct spectrum *sp, int argc, const char **argv)
{
    sws_ap_bool("-s",           &sp->option_server_cycle,  "work as server.");
    sws_ap_int("--server-port", &sp->option_server_port,    "set the server listen port.");
    sws_ap_str("--server-host", &sp->option_server_host,    "set the server listen host.");
    sws_ap_func("-l",           &sp->file_logs,   spectrum_args_append, "set log file, can set multi times.");
    sws_ap_str("-p",            &sp->file_pattern,        "set pattern file.");
    sws_ap_str("-r",            &sp->file_rc,             "set rc.lua file. default is ./spectrum.lua.");
    sws_ap_str("-c",            &sp->option_client_cmd,   "set the client cmd and work as client.");
    sws_ap_int("--log",         &sp->option_log_level,   "set log level: 0-5 err, info, debug");
    sws_ap_bool("--json",       &sp->option_src_type,   "set src type for json");
    sws_ap_int("--slice",       &sp->option_slice_size,   "set slice size");
    sws_ap_bool("--no-redirect",       &sp->option_redirt_out,   "off stdout and stderr redirect");

    return sws_ap(argc, argv);
}


int main(int argc, const char **argv)
{
    struct spectrum *sp;

    // start
    sp = spectrum_init();
    if (!sp)
    {
        logerr("print sp init fail\n");
        return -1;
    }

    if (0 != spectrum_options(sp, argc, argv)) return -1;

    set_loglevel(sp->option_log_level);

    if(splua_init(sp, sp, &sp->lua_env)) return -1;

    //sp_stage_lua_call(sp->L, "spectrum_config");

    if (sp->option_client_cmd)
    {
        return spectrum_start_client(sp);
    }
    return spectrum_start_server(sp);
}
