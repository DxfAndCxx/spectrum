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

#define debug
#define logerr printf
#define loginfo printf

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
    struct record *current;

    struct record *record;
    struct record *record_tail;

    size_t record_num;
    size_t record_nomatch_num;
    size_t record_errmatch_num;

    const char *log;
    size_t loglen;

    lua_State *L;

    pthread_t tid;

    int *ovector;
    int ovector_n;

    struct spectrum *sp;
    bool flag_drop;
};


struct spectrum{
    struct sws_filebuf *raw_pattern;
    char               *pattern;
    struct string      *fields;
    pcre               *re;
    int                 fields_n;

    unsigned short thread_num;
    struct sp_thread *threads;

    lua_State *L;

    const char *file_log;
    const char *file_pattern;
    const char *file_rc;

    struct sws_filebuf *log_buf;

    int server_fd;
    int confd;

    // options
    int option_work_as_server;
    int option_server_cycle;
    int option_server_port;
    int option_nomatch_output;
    const char *option_server_host;
    const char *option_client_cmd;

};


void *record_reads(void *);
void *record_iter(void *_);
void *record_vars_append(struct record *record, enum var_type type, unsigned int size);
struct item *record_vars_get(struct record *record, string_t *s);

int pattern_compile(struct spectrum *spectrum, const char *path);

int spectrum_start_server(struct spectrum *sp);
int spectrum_start_client(struct spectrum *sp);

string_t *sp_lua_tolstring(lua_State *L, int index);
int sp_stage_lua_call(lua_State *L, const char *name);

lua_State *splua_init(struct spectrum *sp, void *data);

#endif





