/**
 *   author       :   丁雪峰
 *   time         :   2016-07-23 01:03:44
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "spectrum.h"

static int loglevel;

int sp_lua_tolstring(lua_State *L, int index, string_t *ss)
{
    string_t s;
    s.s = (char *)lua_tolstring(L, index, &s.l);

    ss = Malloc(s.l + sizeof(s) + 1);
    ss->s = (char *)ss + sizeof s;
    ss->l = s.l;
    memcpy(ss->s, s.s, ss->l);
    *(ss->s + ss->l) = 0;
    return 0;
}

int sp_stage_lua_callx(lua_State *L, const char *name, int nargs, int nresults)
{
    lua_getglobal(L, name);
    if (lua_isfunction(L, -1))
    {
        if (0 != lua_pcall(L, nargs, nresults, 0))
        {
            printf("error running function `%s': %s\n", name,
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }
        return 0;
    }

    lua_pop(L, 1);
    return -1;
}

int sp_stage_lua_call(lua_State *L, const char *name)
{
    return sp_stage_lua_callx(L, name, 0, 0);
}

void set_loglevel(int level)
{
    loglevel = level;
}

int __log(int level, const char *fmt, ...)
{
    int n;
    char buf[512];
    va_list ap;

    if (level > loglevel) return 0;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    write(2, buf, n);

    return n;
}


