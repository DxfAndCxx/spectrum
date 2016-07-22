/**
 *   author       :   丁雪峰
 *   time         :   2016-07-21 05:50:53
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __SPECTRUM_H__
#define __SPECTRUM_H__

#include "pcre.h"
#include "sws.h"

#define Malloc malloc
#define Free free
#define OVECCOUNT 50

enum var_type{
    VAR_TYPE_STR=0,
    VAR_TYPE_NUM,
    VAR_TYPE_NUM_ARRAY,
    VAR_TYPE_NONE,
};

struct string{
    char *s;
    unsigned int l;
};

struct number{
    float n;
};

typedef struct string string_t;
typedef struct number number_t;

struct item_string{
    struct string *name;
    struct string s;
    struct item_string *next;
};


struct item_number{
    struct string *name;
    struct number n;
    struct item_number *next;
};

struct item_array{
    struct string *name;
    struct item_array *next;
    unsigned short size;
    struct number n[];
};


struct record{
    struct item_array *array;
    struct item_array *array_tail;

    struct item_number *number;
    struct item_number *number_tail;

    struct item_string *string;
    struct item_string *string_tail;

    struct record *next;
};


struct format{
    struct sws_filebuf *raw_pattern;
    char *pattern;
    struct string *names;

    struct record *record;
    struct record *record_tail;

    pcre *re;
};


int record_reads(struct format *fmt, const char *src, size_t len);

#endif





