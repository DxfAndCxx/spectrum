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
    memset(format, 0, sizeof *format);

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


#if TEST
int main()
{
    struct format *fmt;
    struct sws_filebuf *log_buf;
    struct record *record;
    struct item_string *v;

    log_buf = sws_fileread("t/ngx_logs");

    fmt = compile("t/pattern");
    if (!fmt)
    {
        printf("print fmt compile fail\n");
        return -1;
    }

    record_reads(fmt, log_buf->buf, log_buf->size);

    record = fmt->record;


    while (record)
    {
        v = record->string;
        printf("==================================\n");
        while (v)
        {

            printf("%-15.*s: %.*s\n", v->name->l, v->name->s, v->s.l, v->s.s);
            v = v->next;
        }
        record = record->next;
    }
}
#endif

