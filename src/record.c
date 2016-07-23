/**
 *   author       :   丁雪峰
 *   time         :   2016-07-22 09:29:25
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include "spectrum.h"
static inline void record_lua_record_set(lua_State *L, struct record *record);
static inline struct record *record_lua_record_get(lua_State *L);

static struct record *record_new()
{
    struct record *record;
    record = Malloc(sizeof(*record));

    //record->array = NULL;
    //record->array_tail = NULL;

    //record->number = NULL;
    //record->number_tail = NULL;

    //record->string = NULL;
    //record->string_tail = NULL;

    record->next = NULL;
    record->vars = record->vars_tail = NULL;

    return record;
}


static struct item *record_vars_get(struct record *record, string_t *s)
{
    struct item *item;
    item = record->vars;
    while(item)
    {
        if ((item->name->l == s->l) && (0 == strncmp(item->name->s, s->s, s->l)))
            return item;
        item = item->next;
    }

    return NULL;
}


static void *record_vars_append(struct record *record, enum var_type type, unsigned int size)
{
    struct item *var;
    var = Malloc(sizeof(*var));
    if (NULL == record->vars)
    {
        record->vars = var;
    }
    else{
        record->vars_tail->next = var;
    }
    var->type = type;

    record->vars_tail = var;
    var->next = NULL;
    return var;
}

struct record *record_read(struct spectrum *sp, const char *src, size_t len)
{
    int ovector[OVECCOUNT * 3];
    int rc, i;
    struct item *item;
    struct record *record;

    rc = pcre_exec(sp->re,            // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
            NULL,          // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
            src,           // subject, 输入参数，要被用来匹配的字符串
            len,  // length, 输入参数， 要被用来匹配的字符串的指针
            0,             // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
            0,             // options, 输入参数， 用来指定匹配过程中的一些选项
            ovector,       // ovector, 输出参数，用来返回匹配位置偏移量的数组
            OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
    // 返回值：匹配成功返回非负数，没有匹配返回负数
    if (rc < 0) {                     //如果没有匹配，返回错误信息
        if (rc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
        else
            printf("Matching error %d\n", rc);
        return NULL;
    }

    record = record_new();
    for (i = 1; i < rc; i++) {             //分别取出捕获分组 $0整个正则公式 $1第一个()
        item = record_vars_append(record, VAR_TYPE_STR, 0);
        item->name = sp->names + i - 1;
        item->v.s.s = (char *)src + ovector[2*i];
        item->v.s.l = ovector[2*i+1] - ovector[2*i];
    }

    record_lua_record_set(sp->L, record);
    lua_getglobal(sp->L, "spectrum_record");
    if (0 != lua_pcall(sp->L, 0, 0, 0))
    {
        printf("error running function `spectrum_record': %s\n", lua_tostring(sp->L, -1));
        lua_pop(sp->L, 1);
    }


    if (sp->record)
    {
        sp->record_tail->next = record;
        sp->record_tail = record;
    }
    else{
        sp->record_tail = sp->record = record;
    }
    return record;
}


int record_reads(struct spectrum *sp, const char *src, size_t len)
{
    const char *s, *e;
    sp->record_num = 0;

    s = e = src;

    while (e - src < len)
    {
        if ('\n' == *e)
        {
            record_read(sp, s, e - s);
            s = e + 1;
            sp->record_num += 1;
        }
        ++e;
    }
    printf("RecordNum: %lu\n", sp->record_num);

    return 0;
}

static inline void record_lua_record_set(lua_State *L, struct record *record)
{
    lua_pushlightuserdata(L, record);
    lua_setglobal(L, "__sp_record");
}

static inline struct record *record_lua_record_get(lua_State *L)
{
    struct record *record;

    lua_getglobal(L, "__sp_record");
    record = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return record;
}


static int record_lua_var_get(lua_State *L)
{
    struct record *record;
    struct item *item;
    string_t s;
    record = record_lua_record_get(L);

    if (NULL == record) {
        return luaL_error(L, "no request object found");
    }

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

static int record_lua_var_append(lua_State *L)
{
    struct record *record;
    struct item *item;
    int value_type;
    string_t *s;
    string_t *v;
    const char *msg;
    record = record_lua_record_get(L);

    if (NULL == record) {
        return luaL_error(L, "no request object found");
    }

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

int record_lua_init(lua_State *L)
{
  /*  register reference maps */
    lua_newtable(L);    /* ngx.record */

    lua_createtable(L, 0, 2 /* nrec */); /* metatable for .var */

    lua_pushcfunction(L, record_lua_var_get);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);

    lua_pushcfunction(L, record_lua_var_append);
    lua_setfield(L, -2, "append");

    lua_setfield(L, -2, "record");

    return 0;
}

int record_iter(struct spectrum *sp)
{
    struct record *record;
    record = sp->record;
    while (record)
    {
        record_lua_record_set(sp->L, record);
        lua_getglobal(sp->L, "spectrum_record_iter");
        lua_pcall(sp->L, 0, 0, 0);
        record = record->next;
    }
    return 0;
}
