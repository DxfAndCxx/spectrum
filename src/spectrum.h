/**
 *   author       :   丁雪峰
 *   time         :   2016-07-21 05:50:53
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __SPECTRUM_H__
#define __SPECTRUM_H__

#include <pthread.h>

#include "pcre.h"
#include "sws.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define Malloc malloc
#define Free free
#define OVECCOUNT 50

#define debug
#define logerr printf

enum var_type{
    VAR_TYPE_STR=0,
    VAR_TYPE_NUM,
    VAR_TYPE_NUM_ARRAY,
    VAR_TYPE_NONE,
};

struct string{
    char *s;
    size_t l;
};

struct number{
    float n;
};

typedef struct string string_t;
typedef struct number number_t;

struct item{
    struct string *name;
    union {
        struct string s;
        struct number n;
    }v;
    enum var_type type;
    struct item *next;
};


struct item_number{
    struct string *name;
    struct item_number *next;
};

struct item_array{
    struct string *name;
    struct item_array *next;
    unsigned short size;
    struct number n[];
};


struct record{
//struct item_array *array;
//    struct item_array *array_tail;
//
//    struct item_number *number;
//    struct item_number *number_tail;
//
//    struct item_string *string;
//    struct item_string *string_tail;

    struct item *vars;
    struct item *vars_tail;

    struct record *next;
};

struct sp_thread{
    struct record *record;
    struct record *record_tail;
    size_t record_num;
    size_t record_nomatch_num;
    size_t record_errmatch_num;

    const char *log;
    size_t loglen;

    lua_State *L;

    pthread_t tid;

    struct spectrum *sp;
};


struct spectrum{
    struct sws_filebuf *raw_pattern;
    char               *pattern;
    struct string      *names;
    pcre               *re;

    unsigned short thread_num;
    struct sp_thread *threads;

    lua_State *L;

    const char *file_log;
    const char *file_pattern;
    const char *file_rc;

    struct sws_filebuf *log_buf;

    // options
    int option_work_as_server;
};


int record_lua_init(struct sp_thread *spt);
void *record_reads(void *);
int record_iter(struct sp_thread *spt);

int pattern_compile(struct spectrum *spectrum, const char *path);

string_t *sp_lua_tolstring(lua_State *L, int index);
int sp_stage_lua_call(lua_State *L, const char *name);

#endif





