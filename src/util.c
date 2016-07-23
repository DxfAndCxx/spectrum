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

#include "spectrum.h"

string_t *sp_lua_tolstring(lua_State *L, int index)
{
    string_t s;
    string_t *ss;
    s.s = (char *)lua_tolstring(L, index, &s.l);

    ss = Malloc(s.l + sizeof s);
    ss->s = (char *)ss + sizeof s;
    ss->l = s.l;
    memcpy(ss->s, s.s, ss->l);
    return ss;
}

int sp_stage_lua_call(lua_State *L, const char *name)
{
    lua_getglobal(L, name);
    if (lua_isfunction(L, -1))
    {
        if (0 != lua_pcall(L, 0, 0, 0))
        {
            printf("error running function `%s': %s\n", name,
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }
        return 0;
    }

//    printf("global `%s' is not function\n", name);
    lua_pop(L, 1);
    return -1;
}



