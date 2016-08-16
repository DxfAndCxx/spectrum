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





static int record_read(struct sp_thread *spt, const char *src, int64_t len)
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
        item->name = spt->sp->fields + i - 1;
        item->v.s.s = (char *)src + spt->ovector[2*i];
        item->v.s.l = spt->ovector[2*i+1] - spt->ovector[2*i];
    }

    spt->current = record;
    if (sp_stage_lua_call(spt->L, "spectrum_record_read"))
    {
        return -1;
    }

    if (spt->flag_drop)
    {
        spt->flag_drop = 0;
        ++spt->records_num_droped;
        record_destory(record);
        return 0;
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
                if (record_read(spt, s, e - s))
                {
                    return NULL;
                }
                s = e + 1;
            }
            ++e;
        }
        iterm = iterm->next;
    }

    if (sp_stage_lua_call(spt->L, "spectrum_record_read_end")) return NULL;

    return NULL;
}


struct item *record_vars_get(record_t *record, string_t *s)
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

    spt = _;
    record = spt->record;
    while (record)
    {
        spt->current = record;
        sp_stage_lua_call(spt->L, "spectrum_record_iter");
        record = record->next;
    }

    if(sp_stage_lua_callx(spt->L, "spectrum_summary", 0, 1)) return NULL;

    if (!lua_istable(spt->L, -1)) return NULL;//ignore


    iterm_t *iterm;
    lua_pushnil(spt->L);
    while (lua_next(spt->L, -2))
    {
        if (lua_isnumber(spt->L, -1))
        {
            iterm = Malloc(sizeof(*iterm));
            iterm->name = sp_lua_tolstring(spt->L, -2);
            iterm->v.n.n = lua_tonumber(spt->L, -1);
            iterm->next = NULL;
            if (spt->summary_tail)
            {
                spt->summary_tail->next = iterm;
                spt->summary_tail = iterm;
            }
            else{
                spt->summary_head = spt->summary_tail = iterm;
            }
        }
        lua_pop(spt->L, 1);
    }

    return 0;
}
