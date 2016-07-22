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
    int ovector[OVECCOUNT * 3];
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
    printf("###########%d\n", rc);

    record = record_new();
    for (i = 1; i < rc; i++) {             //分别取出捕获分组 $0整个正则公式 $1第一个()
        item = record_append(record, VAR_TYPE_STR, 0);
        item->name = fmt->names + i - 1;
        item->s.s = (char *)src + ovector[2*i];
        item->s.l = ovector[2*i+1] - ovector[2*i];
    }
    return record;
}

struct pattern_line{
    const char *name;
    size_t name_len;

    const char *pattern;
    size_t pattern_len;

    const char *end;
};


static int read_one_pattern(struct pattern_line *line, const char *l)
{
    const char *split;
    const char *s;

    s = l;
    while (*s != '\n' && *s != 0) ++s;
    line->end = s;

    split = l;
    while (*split != ':' && split < line->end) ++split;
    if (':' != *split) return -1; // not found :

    if (line->end - split < 2)
        return -2; // pattern is empty

    line->pattern = split + 1;
    line->pattern_len = s - split - 1;

    line->name = NULL;

    s = l;
    while (s < split)
    {
        if (!line->name)
        {
            if (*s != ' ' && *s != '\t')
            {
                line->name = s;
                line->name_len = split - line->name;
            }
        }
        else{
            if (*s == ' ' || *s == '\t')
            {
                line->name_len = s - line->name;
                break;
            }
        }
        ++s;
    }

    return 0;// OK
}



static int read_pattern(struct format *format)
{
    const char *start;
    char *pos;
    int name_index;
    struct pattern_line pl;
    int line_number = 0;
    int res;

    int count = 2;
    start = format->raw_pattern->buf;
    while (*start)
    {
        if ('\n' == *start) ++count;
        ++start;
    }

    format->pattern = Malloc(format->raw_pattern->size);
    format->names =  Malloc(sizeof(struct string) * count);
    memset(format->pattern, 0, format->raw_pattern->size);
    memset(format->names, 0, sizeof(struct string) * count);
    name_index = 0;

    start = format->raw_pattern->buf;
    pos = format->pattern;


    while (*start)
    {
        res = read_one_pattern(&pl, start);
        if (res < 0)
            goto end;

        if (pl.name) *pos++ = '(';

        memcpy((void *)pos, pl.pattern, pl.pattern_len);
        pos += pl.pattern_len;

        if (pl.name)
        {
            *pos++ = ')';

            format->names[name_index].l = pl.name_len;
            format->names[name_index].s = (char *)pl.name;
            ++name_index;
        }
end:
        if (0 == *pl.end) break;
        start = pl.end + 1;
        ++line_number;
    }
    return 0;
}


struct format *compile(const char *path)
{
    struct format *format;
    const char *error;
    int erroffset;

    format = Malloc(sizeof *format);
    format->raw_pattern = sws_fileread(path);

    read_pattern(format);

    printf("Pattern: /%s/\n", format->pattern);

    format->re = pcre_compile(format->pattern,       // pattern, 输入参数，将要被编译的字符串形式的正则表达式
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
    struct sws_filebuf *log_buf;
    struct record *record;

    log_buf = sws_fileread("t/ngx_logs");

    fmt = compile("t/pattern");
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
        printf("%-15.*s: %.*s\n", v->name->l, v->name->s, v->s.l, v->s.s);
        v = v->next;
    }
}
#endif

