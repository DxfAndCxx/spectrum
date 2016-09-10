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

#include "jansson.h"

static inline void record_lua_spt_set(lua_State *L, struct sp_thread *sp);
static inline struct sp_thread *record_lua_spt_get(lua_State *L);

static record_t *record_new()
{
    record_t *record;
    record = Malloc(sizeof(*record));

    record->next = NULL;
    record->vars = record->vars_tail = NULL;

    return record;
}


//just can be destory in record_reads
static void record_destory(record_t *record)
{
    struct item *item;
    struct item *t;
    item = record->vars;
    while (item)
    {
        t = item;
        item = t->next;
        free(t);
    }

    free(record);
}

static int record_append(struct sp_thread *spt, record_t *record)
{
    lua_State *L;
    script_t *script;

    script = spt->lua_env.scripts_read;
    L = spt->lua_env.L;

    spt->current = record;

    if (script)
    {
        lua_settop(L, 0);
        lua_getglobal(L, "scripts");
        lua_getfield(L, -1, script->name);
        lua_getfield(L, -1, "read");
        splua_pcall(L, 0, 0);

        if (spt->flag_drop)
        {
            spt->flag_drop = 0;
            ++spt->records_num_droped;
            record_destory(record);
            return 0;
        }
    }

    ++spt->records_num;
    if (spt->record)
    {
        spt->record_tail->next = record;
        spt->record_tail = record;
    }
    else{
        spt->record_tail = spt->record = record;
    }
    return 0;
}


static int record_read_json_object(struct sp_thread *spt, record_t *record, json_t *object, const char *prefix)
{
    struct item *item;
    const char *k;
    json_t *v;
    int prefix_len;
    json_type type;

    if (prefix)
        prefix_len = strlen(prefix);

    json_object_foreach(object, k, v) {
        type = json_typeof(v);
        if (JSON_OBJECT == type)
        {
            record_read_json_object(spt, record, v, k);
            continue;
        }

        if (JSON_STRING == type)
            item = record_vars_append(record, VAR_TYPE_STR, 0);
        else
            item = record_vars_append(record, VAR_TYPE_NUM, 0);

        if (prefix)
        {
            item->name.l = strlen(k) + prefix_len + 1;
            item->name.s = malloc(item->name.l);

            memcpy(item->name.s, prefix, prefix_len);
            *(item->name.s + prefix_len) = '_';
            memcpy(item->name.s + prefix_len + 1, k, item->name.l - prefix_len - 1);
        }
        else{
            item->name.s = (char *)k;
            item->name.l = strlen(k);
        }

        switch (type) {
            case JSON_STRING:
                item->v.s.s = (char *)json_string_value(v);
                item->v.s.l = strlen(item->v.s.s);
                break;

            case JSON_INTEGER:
                item->v.n.n = json_integer_value(v);
                break;

            case JSON_REAL:
                item->v.n.n = json_real_value(v);
                break;

            case JSON_TRUE:
                item->v.n.n = 1;
                break;

            case JSON_FALSE:
                item->v.n.n = 0;
                break;

            default:
                item->v.n.n = 0;
        }
    }
    return 0;
}


static int record_read_json(struct sp_thread *spt, const char *src, int64_t len)
{
    json_t *j;
    record_t *record;

    j = json_loadb(src, len, 0, NULL);
    if (!j)
    {
        ++spt->records_num_errmatch;
        return -1;
    }

    record = record_new();
    record_read_json_object(spt, record, j, NULL);
    record_append(spt, record);
    return 0;
}


static int record_read_pcre(struct sp_thread *spt, const char *src, int64_t len)
{
    int rc, i;
    struct item *item;
    record_t *record;

    rc = pcre_exec(spt->sp->re,            // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
            NULL,          // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
            src,           // subject, 输入参数，要被用来匹配的字符串
            len,  // length, 输入参数， 要被用来匹配的字符串的指针
            0,             // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
            0,             // options, 输入参数， 用来指定匹配过程中的一些选项
            spt->ovector,       // ovector, 输出参数，用来返回匹配位置偏移量的数组
            spt->ovector_n);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
    // 返回值：匹配成功返回非负数，没有匹配返回负数
    if (rc < 0) {                     //如果没有匹配，返回错误信息
        if (rc == PCRE_ERROR_NOMATCH){
            ++spt->records_num_nomatch;
            if (spt->sp->option_nomatch_output)
                loginfo("%.*s\n", (int)len, src);
        }
        else{
            loginfo("Matching error %d\n", rc);
            ++spt->records_num_errmatch;
        }
        return 0;
    }

    record = record_new();
    for (i = 1; i < rc; i++) {             //分别取出捕获分组 $0整个正则公式 $1第一个()
        item = record_vars_append(record, VAR_TYPE_STR, 0);
        item->name = *(spt->sp->fields + i - 1);
        item->v.s.s = (char *)src + spt->ovector[2*i];
        item->v.s.l = spt->ovector[2*i+1] - spt->ovector[2*i];
    }

    return record_append(spt, record);
}


static int record_read(struct sp_thread *spt, const char *src, int64_t len)
{
    if (0 == spt->sp->option_src_type)
    {
        record_read_pcre(spt, src, len);
    }
    else{
        record_read_json(spt, src, len);
    }

}


void *record_reads(void *_spt)
{
    const char *s, *e;
    struct sp_thread *spt;
    iterm_t *iterm;

    spt = (struct sp_thread*)_spt;

    iterm = spt->logs;
    while (iterm)
    {
        s = e = iterm->v.s.s;

        while (e - iterm->v.s.s < iterm->v.s.l)
        {
            if ('\n' == *e || 0 == *e)
            {
                ++spt->lines_num;
                record_read(spt, s, e - s);
                s = e + 1;
            }
            ++e;
        }
        iterm = iterm->next;
    }

    //if (sp_stage_lua_call(spt->L, "spectrum_record_read_end")) return NULL;

    return NULL;
}


struct item *record_vars_get(record_t *record, string_t *s)
{
    struct item *item;
    item = record->vars;
    while(item)
    {
        if ((item->name.l == s->l) && (0 == strncmp(item->name.s, s->s, s->l)))
            return item;
        item = item->next;
    }

    return NULL;
}


void *record_vars_append(record_t *record, enum var_type type, unsigned int size)
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


void *record_iter(void *_)
{
    struct sp_thread *spt;
    record_t *record;
    lua_State *L;
    script_t **script;
    int drop;

    spt = _;
    record = spt->record;

    L = spt->lua_env.L;

    lua_settop(L, 0);
    lua_getglobal(L, "scripts");

    while (record)
    {
        spt->current = record;
        if (spt->lua_env.scripts_filter)
        {
            lua_getfield(L, -1, spt->lua_env.scripts_filter->name);
            lua_getfield(L, -1, "filter");
            splua_pcall(L, 0, 1);
            drop = 0;
            if (lua_isnil(L, -1))
                drop = 1;

            lua_pop(L, 2);
            if (drop) {
                goto next;
                continue;
            }
        }

        script = spt->lua_env.scripts_iter;
        while (*script)
        {
            printf("scirpt : %p\n", script);
            lua_getfield(L, -1, (*script)->name);
            lua_getfield(L, -1, "iter");
            splua_pcall(L, 0, 0);
            lua_pop(L, 1);
            ++script;
        }

next:
        record = record->next;
    }

    lua_settop(L, 0);
    lua_getglobal(L, "scripts");

    script = spt->lua_env.scripts_map;
    while (*script)
    {
        lua_getfield(L, 1, (*script)->name);
        lua_getfield(L, -1, "map");
        if (0 == splua_pcall(L, 0, 1))
        {
            lua_remove(L, -2);
            (*script)->level = lua_gettop(L);
        }

        ++script;
    }
    return 0;
}
