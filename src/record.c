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

#define loginfo printf

static inline void record_lua_spt_set(lua_State *L, struct sp_thread *sp);
static inline struct sp_thread *record_lua_spt_get(lua_State *L);

static struct record *record_new()
{
    struct record *record;
    record = Malloc(sizeof(*record));

    record->next = NULL;
    record->vars = record->vars_tail = NULL;

    return record;
}


//just can be destory in record_reads
static void record_destory(struct record *record)
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





struct record *record_read(struct sp_thread *spt, const char *src, size_t len)
{
    int ovector[OVECCOUNT * 3];
    int rc, i;
    struct item *item;
    struct record *record;

    rc = pcre_exec(spt->sp->re,            // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
            NULL,          // extra, 输入参数，用来向pcre_exec传一些额外的数据信息的结构的指针
            src,           // subject, 输入参数，要被用来匹配的字符串
            len,  // length, 输入参数， 要被用来匹配的字符串的指针
            0,             // startoffset, 输入参数，用来指定subject从什么位置开始被匹配的偏移量
            0,             // options, 输入参数， 用来指定匹配过程中的一些选项
            ovector,       // ovector, 输出参数，用来返回匹配位置偏移量的数组
            OVECCOUNT);    // ovecsize, 输入参数， 用来返回匹配位置偏移量的数组的最大大小
    // 返回值：匹配成功返回非负数，没有匹配返回负数
    if (rc < 0) {                     //如果没有匹配，返回错误信息
        if (rc == PCRE_ERROR_NOMATCH){
            spt->record_nomatch_num += 1;
        }
        else{
            loginfo("Matching error %d\n", rc);
            spt->record_errmatch_num += 1;
        }
        return NULL;
    }

    record = record_new();
    for (i = 1; i < rc; i++) {             //分别取出捕获分组 $0整个正则公式 $1第一个()
        item = record_vars_append(record, VAR_TYPE_STR, 0);
        item->name = spt->sp->names + i - 1;
        item->v.s.s = (char *)src + ovector[2*i];
        item->v.s.l = ovector[2*i+1] - ovector[2*i];
    }

    spt->current = record;
    sp_stage_lua_call(spt->L, "spectrum_record_read");

    if (spt->flag_drop)
    {
        spt->flag_drop = 0;
        record_destory(record);
        return NULL;
    }

    spt->record_num += 1;
    if (spt->record)
    {
        spt->record_tail->next = record;
        spt->record_tail = record;
    }
    else{
        spt->record_tail = spt->record = record;
    }
    return record;
}


void *record_reads(void *_spt)
{
    const char *s, *e;
    struct sp_thread *spt;
    spt = (struct sp_thread*)_spt;

    s = e = spt->log;

    while (e - spt->log < (long)spt->loglen)
    {
        if ('\n' == *e)
        {
            record_read(spt, s, e - s);
            s = e + 1;
        }
        ++e;
    }
    loginfo("Records Match: %lu NoMatch: %lu ErrMatch: %lu\n",
            spt->record_num, spt->record_nomatch_num, spt->record_errmatch_num);

    return NULL;
}


struct item *record_vars_get(struct record *record, string_t *s)
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


void *record_vars_append(struct record *record, enum var_type type, unsigned int size)
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
    struct record *record;

    spt = _;
    record = spt->record;
    while (record)
    {
        spt->current = record;
        sp_stage_lua_call(spt->L, "spectrum_record_iter");
        record = record->next;
    }

    sp_stage_lua_call(spt->L, "spectrum_record_iter_end");
    return 0;
}
