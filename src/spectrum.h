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




int loginfo(const char *fmt, ...);

#define debug
#define logerr loginfo

enum var_type{
    VAR_TYPE_STR=0,
    VAR_TYPE_NUM,
    VAR_TYPE_NUM_ARRAY,
    VAR_TYPE_NONE,
};

struct string{
    char *s;
    int64_t l;
};

struct number{
    double n;
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

typedef struct dict record_t;
typedef struct item iterm_t;

struct dict{
    struct item *vars;
    struct item *vars_tail;

    record_t *next;
};




struct sp_thread{
    record_t *current;
    record_t *record;
    record_t *record_tail;

    int64_t records_num;
    int64_t records_num_nomatch;
    int64_t records_num_errmatch;
    int64_t records_num_droped;

    iterm_t *logs;

    lua_State *L;

    pthread_t tid;

    int *ovector;
    int ovector_n;

    iterm_t *summary_head;
    iterm_t *summary_tail;

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

    unsigned long time;

    const char *file_pattern;
    const char *file_rc;

    struct sws_filebuf *log_buf;

    iterm_t *file_logs;
    int file_logs_n;

    int server_fd;
    int confd;



    // options
    int option_work_as_server;
    int option_server_cycle;
    int option_server_port;
    int option_nomatch_output;
    int option_slice_size;
    const char *option_server_host;
    const char *option_client_cmd;

};


void *record_reads(void *);
void *record_iter(void *_);
void *record_vars_append(record_t *record, enum var_type type, unsigned int size);
struct item *record_vars_get(record_t *record, string_t *s);

int pattern_compile(struct spectrum *spectrum, const char *path);

int spectrum_start_server(struct spectrum *sp);
int spectrum_start_client(struct spectrum *sp);

string_t *sp_lua_tolstring(lua_State *L, int index);
int sp_stage_lua_call(lua_State *L, const char *name);
int sp_stage_lua_callx(lua_State *L, const char *name, int nargs, int nresults);

lua_State *splua_init(struct spectrum *sp, void *data);

#endif





