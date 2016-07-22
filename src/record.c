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

static struct record *record_new()
{
    struct record *record;
    record = Malloc(sizeof(*record));

    record->array = NULL;
    record->array_tail = NULL;

    record->number = NULL;
    record->number_tail = NULL;

    record->string = NULL;
    record->string_tail = NULL;

    record->next = NULL;

    return record;
}


static void *record_append(struct record *record, enum var_type type, unsigned int size)
{
    if (VAR_TYPE_STR == type)
    {
        struct item_string *var;
        var = Malloc(sizeof(struct item_string));
        if (NULL == record->string)
        {
            record->string = var;
        }
        else{
            record->string_tail->next = var;
        }

        record->string_tail = var;
        var->next = NULL;
        return var;
    }

    if (VAR_TYPE_NUM == type)
    {
        struct item_number *var;
         var = Malloc(sizeof(struct item_number));
         if (NULL == record->number)
         {
             record->number = var;
         }
         else{
             record->number_tail->next = var;
         }
         record->number_tail = var;
        var->next = NULL;
         return var;
    }

    if (VAR_TYPE_NUM_ARRAY == type)
    {
        struct item_array *var;
         var = Malloc(sizeof(struct item_array) + sizeof(struct number) * size);
         if (NULL == record->array)
         {
             record->array = var;
         }
         else{
             record->array_tail->next = var;
         }
         record->array_tail = var;
        var->next = NULL;
         return var;
    }
    return NULL;
}

struct record *record_read(struct spectrum *sp, const char *src, size_t len)
{
    int ovector[OVECCOUNT * 3];
    int rc, i;
    struct item_string *item;
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
        item = record_append(record, VAR_TYPE_STR, 0);
        item->name = sp->names + i - 1;
        item->s.s = (char *)src + ovector[2*i];
        item->s.l = ovector[2*i+1] - ovector[2*i];
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

    s = e = src;

    while (e - src < len)
    {
        if ('\n' == *e)
        {
            record_read(sp, s, e - s);
            s = e + 1;
        }
        ++e;
    }

    return 0;
}
