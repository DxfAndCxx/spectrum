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


static int read_one_pattern(struct pattern_line *line, const char *l, int linenu)
{
    const char *split;
    const char *s;
    const char *origin_line;

    origin_line = l;
    while (' ' == *l || '\t' == *l) ++l;// skip white char in the line head

    s = l;
    while (*s != '\n' && *s != 0) ++s; // get the end
    line->end = s;

    if (line->end - l < 2) return 1; // ignore
    if ('#' == *l) return 1; // ignore

    split = l;
    while (*split != ':' && split < line->end) ++split;
    if (':' != *split){
        logerr("Pattern Err: linenu: %d not found split char [:]\n", linenu);
        return -1; // not found :
    }

    if (line->end - split < 2)
        return -2; // pattern is empty

    line->pattern = split + 1;
    line->pattern_len = s - split - 1;

    line->name = NULL;

    if (':' == *l) return 0;

    line->name = l;
    line->name_len = split - line->name;

    s = l;
    while (s < split)
    {
        if (*s == ' ' || *s == '\t')
        {
            line->name_len = s - line->name;
            break;
        }
        ++s;
    }

    return 0;// OK
}



static int read_pattern(struct spectrum *sp)
{
    const char *start;
    char *pos;
    int name_index;
    struct pattern_line pl;
    int line_number = 0;
    int res;

    int count = 2;
    start = sp->raw_pattern->buf;
    while (*start)
    {
        if ('\n' == *start) ++count;
        ++start;
    }

    sp->pattern = Malloc(sp->raw_pattern->size);
    sp->fields =  Malloc(sizeof(struct string) * count);
    memset(sp->pattern, 0, sp->raw_pattern->size);
    memset(sp->fields, 0, sizeof(struct string) * count);
    name_index = 0;

    start = sp->raw_pattern->buf;
    pos = sp->pattern;


    while (*start)
    {
        ++line_number;
        res = read_one_pattern(&pl, start, line_number);

        if (res < 0)
            return -1;

        if (res) goto end;

        if (pl.name) *pos++ = '(';

        memcpy((void *)pos, pl.pattern, pl.pattern_len);
        pos += pl.pattern_len;

        if (pl.name)
        {
            *pos++ = ')';

            sp->fields[name_index].l = pl.name_len;
            sp->fields[name_index].s = (char *)pl.name;
            ++name_index;
        }
end:
        if (0 == *pl.end) break;
        start = pl.end + 1;
    }

    sp->fields_n = name_index;
    return 0;
}


int pattern_compile(struct spectrum *sp, const char *path)
{
    const char *error;
    int erroffset;

    sp->raw_pattern = sws_fileread(path);
    if (!sp->raw_pattern)
    {
        logerr("read `%s' fail\n", path);
        return -1;
    }

    if (read_pattern(sp)) return -1;

    printf("Pattern: /%s/\n", sp->pattern);

    sp->re = pcre_compile(sp->pattern,       // pattern, 输入参数，将要被编译的字符串形式的正则表达式
                      0,            // options, 输入参数，用来指定编译时的一些选项
                      &error,       // errptr, 输出参数，用来输出错误信息
                      &erroffset,   // erroffset, 输出参数，pattern中出错位置的偏移量
                      NULL);        // tableptr, 输入参数，用来指定字符表，一般情况用NULL

    // 返回值：被编译好的正则表达式的pcre内部表示结构
    if (error) {                 //如果编译失败，返回错误信息
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        return -1;
    }
    return 0;
}

