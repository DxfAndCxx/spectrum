#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"

typedef unsigned char byte;
typedef unsigned int uint;
#define B2IL(b) (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000) | (((b)[3] << 24) & 0xFF000000))
#define B2IU(b) (((b)[3] & 0xFF) | (((b)[2] << 8) & 0xFF00) | (((b)[1] << 16) & 0xFF0000) | (((b)[0] << 24) & 0xFF000000))

struct {
    byte *data;
    byte *index;
    uint *flag;
    uint offset;
} ipip;

static int destroy() {
    if (!ipip.offset) {
        return 0;
    }
    free(ipip.flag);
    free(ipip.index);
    free(ipip.data);
    ipip.offset = 0;
    return 0;
}

static int init(lua_State *L) {
    if (ipip.offset)
        return 0;

    if (!lua_isstring(L, -1))
    {
        lua_pushnil(L);
        lua_pushstring(L, "need db path.");
        return 2;
    }

    FILE *file = fopen(lua_tostring(L, -1), "rb");
    lua_pop(L, 1);

    if (NULL == file)
    {
        lua_pushnil(L);
        lua_pushstring(L, "open db path fail.");
        return 2;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    ipip.data = (byte *) malloc(size * sizeof(byte));
    size_t r = fread(ipip.data, sizeof(byte), (size_t) size, file);

    if (r == 0) {
        return 0;
    }

    fclose(file);

    uint length = B2IU(ipip.data);

    ipip.index = (byte *) malloc(length * sizeof(byte));
    memcpy(ipip.index, ipip.data + 4, length);

    ipip.offset = length;

    ipip.flag = (uint *) malloc(256 * sizeof(uint));
    memcpy(ipip.flag, ipip.index, 256 * sizeof(uint));

    return 0;
}

static int find(lua_State *L) {
    const char *ip;
    char result[1024];

    if (!lua_isstring(L, -1))
    {
        lua_pushnil(L);
        lua_pushstring(L, "need ip.");
        return 2;
    }

    ip = lua_tostring(L, -1);
    lua_pop(L, 1);


    uint ips[4];
    int num = sscanf(ip, "%d.%d.%d.%d", &ips[0], &ips[1], &ips[2], &ips[3]);
    if (num != 4) {
        lua_pushnil(L);
        lua_pushstring(L, "ip format err");
        return 2;
    }

    uint ip_prefix_value = ips[0];
    uint ip2long_value = B2IU(ips);
    uint start = ipip.flag[ip_prefix_value];
    uint max_comp_len = ipip.offset - 1028;
    uint index_offset = 0;
    uint index_length = 0;
    for (start = start * 8 + 1024; start < max_comp_len; start += 8) {
        if (B2IU(ipip.index + start) >= ip2long_value) {
            index_offset = B2IL(ipip.index + start + 4) & 0x00FFFFFF;
            index_length = ipip.index[start + 7];
            break;
        }
    }

    memcpy(result, ipip.data + ipip.offset + index_offset - 1024, index_length);
    result[index_length] = '\0';

    printf("%s\n", result);

    lua_newtable(L);

    char *s; char *pos;
    int i;
    pos = s = result;

    i = 1;
    while (*pos)
    {
        if (*pos == '\t')
        {
            *pos = 0;
            lua_pushstring(L, s);
            lua_rawseti(L , -2, i++);

            s = pos + 1;
        }
        ++pos;
    }
    lua_pushstring(L, s);
    lua_rawseti(L , -2, i++);

    return 1;
}

static const struct luaL_reg thislib[] = {
    {"init", init},
    {"find", find},
    {NULL, NULL}
};

LUALIB_API int luaopen_ipip(lua_State *L)
{
    luaL_register(L, "ipip", thislib);


    return 1;
}
