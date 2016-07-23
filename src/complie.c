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



static int read_pattern(struct spectrum *spectrum)
{
    const char *start;
    char *pos;
    int name_index;
    struct pattern_line pl;
    int line_number = 0;
    int res;

    int count = 2;
    start = spectrum->raw_pattern->buf;
    while (*start)
    {
        if ('\n' == *start) ++count;
        ++start;
    }

    spectrum->pattern = Malloc(spectrum->raw_pattern->size);
    spectrum->names =  Malloc(sizeof(struct string) * count);
    memset(spectrum->pattern, 0, spectrum->raw_pattern->size);
    memset(spectrum->names, 0, sizeof(struct string) * count);
    name_index = 0;

    start = spectrum->raw_pattern->buf;
    pos = spectrum->pattern;


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

            spectrum->names[name_index].l = pl.name_len;
            spectrum->names[name_index].s = (char *)pl.name;
            ++name_index;
        }
end:
        if (0 == *pl.end) break;
        start = pl.end + 1;
        ++line_number;
    }
    return 0;
}


int pattern_compile(struct spectrum *spectrum, const char *path)
{
    const char *error;
    int erroffset;

    spectrum->raw_pattern = sws_fileread(path);

    read_pattern(spectrum);

    printf("Pattern: /%s/\n", spectrum->pattern);

    spectrum->re = pcre_compile(spectrum->pattern,       // pattern, 输入参数，将要被编译的字符串形式的正则表达式
                      0,            // options, 输入参数，用来指定编译时的一些选项
                      &error,       // errptr, 输出参数，用来输出错误信息
                      &erroffset,   // erroffset, 输出参数，pattern中出错位置的偏移量
                      NULL);        // tableptr, 输入参数，用来指定字符表，一般情况用NULL

    // 返回值：被编译好的正则表达式的pcre内部表示结构
    if (error) {                 //如果编译失败，返回错误信息
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        free(spectrum);
        return -1;
    }
    return 0;
}

