/**
 *   author       :   丁雪峰
 *   time         :   2016-07-24 14:57:27
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "spectrum.h"

int spectrum_start_client(struct spectrum *sp)
{
    int fd;
    char buf[8192];
    int cmdlen;
    int res;

    if (!sp->option_client_cmd)
    {
        logerr("you should set the cmd by -c\n");
        return -1;
    }
    cmdlen = strlen(sp->option_client_cmd);

    if (cmdlen > 1000)
    {
        logerr("CMD Too Long: %s\n", sp->option_client_cmd);
        return -1;
    }

    fd = sws_net_connect(sp->option_server_host, sp->option_server_port);

    memcpy(buf, sp->option_client_cmd, cmdlen);

    buf[cmdlen] = '\r';
    buf[cmdlen + 1] = '\n';


    write(fd, buf, cmdlen + 2);

    while (1)
    {
        res = read(fd, buf, sizeof(buf));
        if (0 == res) break;
        write(1, buf, res);
    }

    close(fd);

    return 0;
}
