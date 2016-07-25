/**
 *   author       :   丁雪峰
 *   time         :   2016-07-25 02:30:31
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "spectrum.h"

static inline void *splua_get_data(lua_State *L)
{
    void *data;

    lua_getglobal(L, "__sp_data");
    data = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return data;
}



static int record_lua_drop(lua_State *L)
{
    struct sp_thread *spt;
    spt = splua_get_data(L);

    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }

    spt->flag_drop = 1;

    return 0;
}


static int record_lua_var_get(lua_State *L)
{
    struct record *record;
    struct item *item;
    struct sp_thread *spt;
    string_t s;
    spt = splua_get_data(L);

    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }
    record = spt->current;

    s.s = (char *)lua_tolstring(L, -1, &s.l);

    item = record_vars_get(record, &s);
    if (!item)
        return 0;

    switch(item->type)
    {
        case VAR_TYPE_STR:
            lua_pushlstring(L, item->v.s.s, item->v.s.l);
            return 1;
        default:
            return 0;

    }
}

static int record_lua_append(lua_State *L)
{
    struct record *record;
    struct item *item;
    int value_type;
    struct sp_thread *spt;
    string_t *s;
    string_t *v;
    const char *msg;
    spt = splua_get_data(L);

    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }

    record = spt->current;

    if (lua_type(L, 1) != LUA_TSTRING) {
        return luaL_error(L, "bad variable name");
    }


    s = sp_lua_tolstring(L, 1);

    value_type = lua_type(L, 2);
    switch (value_type) {
    case LUA_TNUMBER:
    case LUA_TSTRING:
        v = sp_lua_tolstring(L, 2);

        item = record_vars_append(record, VAR_TYPE_STR, 0);
        item->name = s;
        item->v.s = *v;
        break;

    case LUA_TNIL:
        /* undef the variable */
        return 0;

    default:
        msg = lua_pushfstring(L, "string, number, or nil expected, "
                              "but got %s", lua_typename(L, value_type));
        return luaL_argerror(L, 1, msg);
    }

    return 0;
}


static int spectrum_lua_var_set(lua_State *L)
{
    struct spectrum *sp;
    string_t *field;
    string_t *value;

    sp = splua_get_data(L);
    if (NULL == sp) {
        return luaL_error(L, "no request object found");
    }

    field = sp_lua_tolstring(L, 2);
    value = sp_lua_tolstring(L, 3);

    if (0 == strncmp("file_log", field->s, field->l))
    {
        sp->file_log = value->s;
        //debug("sp.file_log = %s\n", sp->file_log);
        return 0;
    }

    if (0 == strncmp("file_pattern", field->s, field->l))
    {
        sp->file_pattern = value->s;
        //debug("sp.file_pattern = %s\n", sp->file_pattern);
        return 0;
    }

    return luaL_error(L, "sp no know field: %.*s\n", field->l, field->s);
}


static int spectrum_lua_pattern_index(lua_State *L)
{
    struct spectrum *sp;
    struct sp_thread *spt;
    string_t *field;
    int i;
    string_t *name;

    spt = splua_get_data(L);
    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }

    field = sp_lua_tolstring(L, 2);
    if (!strncmp("fields", field->s, field->l))
    {
        sp = spt->sp;

        lua_newtable(L);

        name = sp->fields;
        i = 1;
        while (name->s)
        {
            lua_pushlstring(L, name->s, name->l);
            lua_rawseti(L, -2, i);
            ++i;
            ++name;
        }

        return 1;
    }

    return 0;
}


static void splua_init_set_sp(lua_State *L)
{
    // create sp
    lua_createtable(L, 0 /* narr */, 116 /* nrec */);    /* sp.* */
    lua_setglobal(L, "sp");

    // create sp metatable
    lua_getglobal(L, "sp");
    lua_newtable(L);    /* sp.vars */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, spectrum_lua_var_set);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "opt");

    lua_settop(L, 0);
}


static void splua_init_set_pattern(lua_State *L)
{
    // create sp metatable
    lua_getglobal(L, "sp");
    lua_newtable(L);    /* sp.vars */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, spectrum_lua_pattern_index);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "pattern");

    lua_settop(L, 0);
}


static void splua_init_set_record(lua_State *L)
{
    // create sp.record
    lua_getglobal(L, "sp");

    lua_newtable(L);    /* sp.record */

    // set method for sp.record
    //lua_getfield(L, -1, "record");

    lua_pushcfunction(L, record_lua_append); // Add append method
    lua_setfield(L, -2, "append");

    lua_pushcfunction(L, record_lua_drop); // Add drop method
    lua_setfield(L, -2, "drop");



    // create sp.record.vars
    lua_newtable(L);    /* sp.record.vars */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, record_lua_var_get);
    lua_setfield(L, -2, "__index");


    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "vars");
    lua_setfield(L, -2, "record"); // record end
    lua_settop(L, 0);
}


lua_State *splua_init(struct spectrum *sp, void *data)
{
    lua_State *L;

    if (0 != access(sp->file_rc, R_OK))
    {
        logerr("cannot access `%s'\n", sp->file_rc);
        return NULL;
    }

    L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushlightuserdata(L, data);
    lua_setglobal(L, "__sp_data");


    splua_init_set_sp(L);
    splua_init_set_pattern(L);
    splua_init_set_record(L);


    if (0 != luaL_dofile(L, sp->file_rc))
    {
        printf("dofile `%s' err: %s\n", sp->file_rc, lua_tostring(L, -1));
        lua_pop(L, 1);
        return NULL;
    }

    return L;
}


