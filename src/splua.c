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


static int record_lua_keys(lua_State *L)
{
    struct sp_thread *spt;
    struct item *item;
    record_t *record;
    int i;

    spt = splua_get_data(L);

    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }

        lua_newtable(L);

    record = spt->current;
    item = record->vars;

    i = 1;
    while(item)
    {
        lua_pushlstring(L, item->name.s, item->name.l);
        lua_rawseti(L, -2, i);
        ++i;
        item = item->next;
    }
    return 1;
}



static int record_lua_var_get(lua_State *L)
{
    record_t *record;
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

        case VAR_TYPE_NUM:
            lua_pushnumber(L, item->v.n.n);
            return 1;
        default:
            return 0;

    }
}

static int record_lua_append(lua_State *L)
{
    record_t *record;
    struct item *item;
    int value_type;
    struct sp_thread *spt;
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



    value_type = lua_type(L, 2);
    switch (value_type) {
    case LUA_TNUMBER:
        item = record_vars_append(record, VAR_TYPE_NUM, 0);
        sp_lua_tolstring(L, 1, &item->name);
        item->v.n.n =  lua_tonumber(L, 2);
        break;

    case LUA_TSTRING:
        item = record_vars_append(record, VAR_TYPE_STR, 0);
        sp_lua_tolstring(L, 1, &item->name);
        sp_lua_tolstring(L, 2, &item->v.s);
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


static int splua_handle_sp_opt_newindex(lua_State *L)
{
    struct spectrum *sp;
    string_t field;
    string_t value;

    sp = splua_get_data(L);
    if (NULL == sp) {
        return luaL_error(L, "no request object found");
    }

    sp_lua_tolstring(L, 2, &field);

    if (0 == strncmp("file_logs", field.s, field.l))
    {
        if (sp->file_logs) return 0;

        iterm_t **iterm;
        iterm = &sp->file_logs;
        if (lua_isstring(L, -1))
        {
            *iterm = Malloc(sizeof(iterm_t));
            sp_lua_tolstring(L, -1, &(*iterm)->name);
            (*iterm)->next = NULL;
            return 0;
        }

        if (lua_istable(L, -1))
        {
            lua_pushnil(sp->lua_env.L);
            while (lua_next(sp->lua_env.L, -2))
            {
                if (lua_isstring(sp->lua_env.L, -1))
                {
                    *iterm = Malloc(sizeof(iterm_t));
                    sp_lua_tolstring(L, -1, &(*iterm)->name);
                    (*iterm)->next = NULL;
                    iterm = &(*iterm)->next;
                }
                lua_pop(sp->lua_env.L, 1);
            }
            return 0;
        }

        return luaL_error(L, "sp file_logs just support string or table.\n");
    }

    if (0 == strncmp("file_pattern", field.s, field.l))
    {
        if (sp->file_pattern) return 0;

        sp_lua_tolstring(L, 3, &value);
        sp->file_pattern = value.s;
        //debug("sp.file_pattern = %s\n", sp->file_pattern);
        return 0;
    }

    return luaL_error(L, "sp no know field: %.*s\n", field.l, field.s);
}


static int splua_handle_patter_index(lua_State *L)
{
    struct spectrum *sp;
    struct sp_thread *spt;
    string_t field;
    int i;
    string_t *name;

    spt = splua_get_data(L);
    if (NULL == spt) {
        return luaL_error(L, "no request object found");
    }

    sp = spt->sp;

    sp_lua_tolstring(L, 2, &field);
    if (!strncmp("fields", field.s, field.l))
    {

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

    if (!strncmp("pattern", field.s, field.l))
    {
        lua_pushstring(L, sp->pattern);
        return 1;
    }

    return 0;
}

static int splua_handle_sp_index(lua_State *L)
{
    struct sp_thread *spt;
    string_t field;

    spt = splua_get_data(L);
    sp_lua_tolstring(L, 2, &field);

    if (!strncmp("num", field.s, field.l))
    {
        lua_pushnumber(L, spt->records_num);
        return 1;
    }

    if (!strncmp("num_droped", field.s, field.l))
    {
        lua_pushnumber(L, spt->records_num_droped);
        return 1;
    }

    if (!strncmp("num_nomatch", field.s, field.l))
    {
        lua_pushnumber(L, spt->records_num_nomatch);
        return 1;
    }

    if (!strncmp("num_errmatch", field.s, field.l))
    {
        lua_pushnumber(L, spt->records_num_errmatch);
        return 1;
    }

    if (!strncmp("threads", field.s, field.l))
    {
        lua_pushnumber(L, spt->sp->thread_num);
        return 1;
    }

    if (!strncmp("time", field.s, field.l))
    {
        lua_pushnumber(L, spt->sp->thread_num);
        return 1;
    }

    return 0;
}


static void splua_init_set_sp(lua_State *L)
{
    // create sp
    lua_createtable(L, 0 /* narr */, 116 /* nrec */);    /* sp.* */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, splua_handle_sp_index);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_setglobal(L, "sp");



    // create sp metatable
    lua_getglobal(L, "sp");
    lua_newtable(L);    /* sp.vars */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, splua_handle_sp_opt_newindex);
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

    lua_pushcfunction(L, splua_handle_patter_index);
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

    lua_pushcfunction(L, record_lua_keys); // Add keys method
    lua_setfield(L, -2, "keys");


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

static void splua_script(script_t ***pos, lua_State *L, const char *path, int *index)
{
    int level;
    int nresult;
    script_t *script;

    lua_getglobal(L, "scripts");
    level = lua_gettop(L);
    if (luaL_dofile(L, path))
    {
        printf("luaL_loadfile `%s' err: %s\n", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return ;
    }
    nresult = lua_gettop(L) - level;
    debug("* loadfile: %s nresult: %d\n", path, nresult);

    while (nresult)
    {
        --nresult;

        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            continue;
        }

        script = malloc(sizeof(*script));
        memset(script, 0, sizeof(*script));

        **pos = script;
        *pos = &script->next;


        //lua_getfield(L, -1, "_order");
        //if (lua_isnumber(L, -1))
        //    script->order = lua_tonumber(L, -1);
        //lua_pop(L, 1);

        lua_getfield(L, -1, "read");
        if (lua_isfunction(L, -1)) script->stages |= STAGE_READ;
        lua_pop(L, 1);

        lua_getfield(L, -1, "iter");
        if (lua_isfunction(L, -1)) script->stages |= STAGE_ITER;
        lua_pop(L, 1);

        lua_getfield(L, -1, "map");
        if (lua_isfunction(L, -1)) script->stages |= STAGE_MAP;
        lua_pop(L, 1);

        lua_getfield(L, -1, "reduce");
        if (lua_isfunction(L, -1)) script->stages |= STAGE_REDUCE;
        lua_pop(L, 1);

        sprintf(script->name, "script_mode_%d", ++*index);
        debug("* Append Mod: %s\n", script->name);
        lua_setfield(L, -2 - nresult, script->name);
    }

    lua_pop(L, 1);
}


static script_t *splua_scripts(const char *dirpath, lua_State *L)
{
    DIR * dir;
    struct dirent * ptr;
    int i = 0;
    char path[1024];
    script_t *head = NULL;
    script_t **pos;
    script_t **ppos;
    script_t *t;

    pos = &head;

    lua_newtable(L);    /* sp.vars */
    lua_setglobal(L, "scripts");

    dir = opendir(dirpath);
    while((ptr = readdir(dir)) != NULL)
    {
        if (DT_REG != ptr->d_type) continue;
        if (ptr->d_name[0] == '.') continue;

        if (strlen(dirpath) + strlen(ptr->d_name) + 10 > sizeof(path))
        {
            logerr("path to %s is too long. Not Load.", ptr->d_name);
            continue;
        }
        strcpy(path, dirpath);
        *(path + strlen(dirpath)) = '/';
        strcpy(path + strlen(dirpath) + 1, ptr->d_name);

        splua_script(&pos, L, path, &i);
    }
    closedir(dir);

    pos = &head;

    while (*pos)
    {
        ppos = &(*pos)->next;
        while(*ppos)
        {
            if ((*pos)->order > (*ppos)->order)
            {
                t = (*pos)->next;
                (*pos)->next = (*ppos)->next;
                (*ppos)->next = t;

                t = *ppos;
                *ppos = *pos;
                *pos = t;
            }
            ppos = &(*ppos)->next;
        }
        pos = &(*pos)->next;
    }
    return head;
}


lua_env_t splua_init(struct spectrum *sp, void *data)
{
    lua_env_t env;
    lua_State *L;

    //if (0 != access(sp->file_rc, R_OK))
    //{
    //    logerr("splua_init: cannot access `%s'\n", sp->file_rc);
    //    return ;
    //}

    L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushlightuserdata(L, data);
    lua_setglobal(L, "__sp_data");


    splua_init_set_sp(L);
    splua_init_set_pattern(L);
    splua_init_set_record(L);

    env.scripts = splua_scripts(sp->file_rc, L);
    env.L = L;

    return env;
}


