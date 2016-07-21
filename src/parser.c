/**
 *   author       :   丁雪峰
 *   time         :   2016-07-20 09:29:19
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#include "spectrum.h"
#include "sws.h"

struct record *record_new()
{
    struct record *record;
    record = Malloc(sizeof(*record));

    record->array = NULL;
    record->array_tail = NULL;

    record->number = NULL;
    record->number_tail = NULL;

    record->string = NULL;
    record->string_tail = NULL;

    return record;
}


void *record_append(struct record *record, enum var_type type, unsigned int size)
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


struct record *record_read(const char *src, size_t len, struct format *fmt)
{
    int ovector[OVECCOUNT * 2];
    int rc, i;
    struct item_string *item;
    struct record *record;

    rc = pcre_exec(fmt->re,            // code, 输入参数，用pcre_compile编译好的正则表达结构的指针
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
        item->s.s = (char *)src + ovector[2*i];
        item->s.l = ovector[2*i+1] - ovector[2*i];
    }
    return record;
}


struct format *compile(const char *pattern)
{
    struct format *format;
    const char *error;
    int erroffset;
    format = Malloc(sizeof *format);

    format->re = pcre_compile(pattern,       // pattern, 输入参数，将要被编译的字符串形式的正则表达式
                      0,            // options, 输入参数，用来指定编译时的一些选项
                      &error,       // errptr, 输出参数，用来输出错误信息
                      &erroffset,   // erroffset, 输出参数，pattern中出错位置的偏移量
                      NULL);        // tableptr, 输入参数，用来指定字符表，一般情况用NULL

    // 返回值：被编译好的正则表达式的pcre内部表示结构
    if (error) {                 //如果编译失败，返回错误信息
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        free(format);
        return NULL;
    }
    return format;

}


#if 1
int main()
{
    struct format *fmt;

    struct sws_filebuf *pattern_buf;
    struct sws_filebuf *log_buf;

    struct record *record;

    pattern_buf = sws_fileread("t/pattern");
    pattern_buf->buf[pattern_buf->size - 1] = 0;
    log_buf = sws_fileread("t/ngx_logs");





    fmt = compile(pattern_buf->buf);
    if (!fmt)
    {
        printf("print fmt compile fail\n");
        return -1;
    }

    record = record_read(log_buf->buf, log_buf->size, fmt);

    if (!record)
        return -1;

    struct item_string *v;

    v = record->string;
    while (v)
    {
        printf("%.*s\n", v->s.l, v->s.s);
        v = v->next;
    }
}
#endif

