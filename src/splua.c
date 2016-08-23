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

static int splua_script(script_t ***pos, lua_State *L, const char *path, int *index)
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
        return -1;
    }
    nresult = lua_gettop(L) - level;
    debug("* Loadfile: %s nresult: %d\n", path, nresult);

    if (nresult < 1) {
        logwrn("* %s no return table.\n");
    }

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


        lua_getfield(L, -1, "_order");
        if (lua_isnumber(L, -1))
            script->order = lua_tonumber(L, -1);
        lua_pop(L, 1);


        sprintf(script->name, "script_mode_%d", ++*index);
        lua_setfield(L, -2 - nresult, script->name);

        debug("* Append Mod: %s\n", script->name);
    }

    lua_pop(L, 1);
    return 0;
}





static int splua_scripts_stage(lua_env_t *env)
{
    lua_State *L;
    script_t *s;
    script_t  **m;

    L = env->L;
    m = malloc(sizeof(void *) * 3 * env->scripts_n);
    memset(m, 0, sizeof(void *) * 3 * env->scripts_n);

    env->scripts_iter = m;
    env->scripts_map = m + env->scripts_n;
    env->scripts_reduce = m + env->scripts_n * 2;

    s = env->scripts;

    while (s)
    {
        lua_getglobal(L, "scripts");
        lua_getfield(L, -1, s->name);

        lua_getfield(L, -1, "read");
        if (lua_isfunction(L, -1))
        {
            if (env->scripts_read)
            {
                logerr("`read' function should just one!\n");
                return -1;
            }
            env->scripts_read = s;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "filter");
        if (lua_isfunction(L, -1))
        {
            if (env->scripts_filter)
            {
                logerr("`filter' function should just one!.");
                return -1;
            }
            env->scripts_filter = s;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "iter");
        if (lua_isfunction(L, -1))
        {
            *env->scripts_iter = s;
            ++env->scripts_iter;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "map");
        if (lua_isfunction(L, -1))
        {
            *env->scripts_map = s;
            ++env->scripts_map;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "reduce");
        if (lua_isfunction(L, -1))
        {
            *env->scripts_reduce = s;
            ++env->scripts_reduce;
        }
        lua_pop(L, 1);

        s = s->next;
    }
    lua_settop(L, 0);


    {
        int t;
        loginfo(" * Stage: ");
        if (env->scripts_read) loginfo("read -> ");
        if (env->scripts_filter) loginfo("filter -> ");

        t = env->scripts_iter - m;
        if (t) loginfo("iter(%d) -> ", t);

        t = env->scripts_map - m - env->scripts_n;
        if (t) loginfo("map(%d) -> ", t);

        t = env->scripts_reduce - m - env->scripts_n * 2;
        if (t) loginfo("reduce(%d) -> ", t);

        loginfo("\n");
    }

    env->scripts_iter = m;
    env->scripts_map = m + env->scripts_n;
    env->scripts_reduce = m + env->scripts_n * 2;
    return 0;
}


static int splua_scripts(lua_env_t *env, const char *dirpath, lua_State *L)
{
    DIR * dir;
    struct dirent * ptr;
    int i = 0;
    char path[1024];
    script_t *head = NULL;
    script_t **pos;
    script_t **ppos;
    script_t *t;

    if (!dirpath) return 0;

    pos = &head;

    lua_newtable(L);    /* sp.vars */
    lua_setglobal(L, "scripts");

    dir = opendir(dirpath);
    if (dir <= 0)
    {
        logerr("opendir fail: %s\n", dirpath);
        return -1;
    }

    while((ptr = readdir(dir)) != NULL)
    {
        if (DT_REG != ptr->d_type) continue;
        if (strlen(ptr->d_name) < 5) continue;
        if (strcmp(ptr->d_name + strlen(ptr->d_name) - 4, ".lua")) continue;
        if (ptr->d_name[0] == '.') continue;

        if (strlen(dirpath) + strlen(ptr->d_name) + 10 > sizeof(path))
        {
            logerr("path to %s is too long. Not Load.", ptr->d_name);
            continue;
        }
        strcpy(path, dirpath);
        *(path + strlen(dirpath)) = '/';
        strcpy(path + strlen(dirpath) + 1, ptr->d_name);

        if (splua_script(&pos, L, path, &i))
            return -1;
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

    env->scripts = head;
    env->scripts_n = i;

    return splua_scripts_stage(env);
}


#include <execinfo.h>

static int splua_panic(lua_State *L)
{
    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    logerr("LUA PANIC:");
    for (j = 0; j < nptrs; j++)
        logerr("%s\n", strings[j]);

    free(strings);
    return 0;
}

int splua_set_path(struct spectrum *sp, lua_State *L)
{
    char path[1024];

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); // get field "path" from table at top of stack (-1)

    strcpy(path, lua_tostring(L, -1));
    strcat(path, ";./?.lua;./modules/?.so;./modules/?.lua");

    lua_pop(L, 1); // get rid of the string on the stack we just pushed on line 5
    lua_pushstring(L, path); // push the new one
    lua_setfield(L, -2, "path"); // set the field "path" in table at -2 with value at top of stack
    lua_pop( L, 1 ); // get rid of package table from top of stack
    return 0; // all done!
}

int splua_init(struct spectrum *sp, void *data, lua_env_t *env)
{
    lua_State *L;
    int res;

    memset(env, 0, sizeof(*env));

    L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushlightuserdata(L, data);
    lua_setglobal(L, "__sp_data");


    splua_init_set_sp(L);
    splua_init_set_pattern(L);
    splua_init_set_record(L);

//    splua_set_path(sp, L);
    lua_atpanic(L, splua_panic);

    env->L = L;
    res = splua_scripts(env, sp->file_rc, L);
    lua_settop(L, 0);

    return res;
}


int splua_copy_table(lua_State *d, lua_State *s, int index)
{
    if (!lua_istable(s, index)){
        logerr("splua_copy_table: index(%d) in stack(%x) is not table.\n",
                index, s);
        return -1;
    }

    if (index < 0)
        index = lua_gettop(s) + 1 + index;

    lua_newtable(d);
    lua_pushnil(s);
    while(lua_next(s, index))
    {
        switch(lua_type(s, -1))
        {
            case LUA_TTABLE:
                splua_copy_table(d, s, -1);
                break;

            case LUA_TSTRING:
                lua_pushstring(d, lua_tostring(s, -1));
                break;

            case LUA_TNUMBER:
                lua_pushnumber(d, lua_tonumber(s, -1));
                break;

            case LUA_TNIL:
                lua_pushnil(d);
                break;

            case LUA_TBOOLEAN:
                lua_pushboolean(d, lua_toboolean(s, -1));
                break;

            default:
                if (lua_isnumber(s, -2))
                    logerr("The type of key: %d "
                            "not support in splua_copy_table.\n",
                            lua_tonumber(s, -2));
                else
                    logerr("The type of key: %s "
                            "not support in splua_copy_table.\n",
                            lua_tostring(s, -2));
                goto fail;


        }

        if (lua_isnumber(s, -2))
            lua_rawseti(d, -2, lua_tonumber(s, -2));
        else
            lua_setfield(d, -2, lua_tostring(s, -2));

fail:
        lua_pop(s, 1); // pop value
    }
    return 0;
}

int _splua_pcall(const char * stack, lua_State *L, int nargs, int nresult)
{
    if(lua_pcall(L, nargs, nresult, 0))
    {
        logerr("Lua Err in `%s': %s\n", stack, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    return 0;
}

