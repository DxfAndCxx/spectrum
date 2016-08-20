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


#define LogLevelErr 0
#define LogLevelInfo 4
#define LogLevelDebug 5


#define STAGE_READ   1
#define STAGE_ITER   (1 < 1)
#define STAGE_MAP    (1 < 2)
#define STAGE_REDUCE (1 < 3)


int __log(int level, const char *fmt, ...);
void set_loglevel(int level);

#define debug(...)  __log(LogLevelDebug, __VA_ARGS__)
#define loginfo(...) __log(LogLevelInfo, __VA_ARGS__)
#define logerr(...) __log(LogLevelErr, __VA_ARGS__)


//得到成员在结构
//#define offsetof(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *)0)->member) * __mptr = (ptr);     \
        (type *)((char *)__mptr - offsetof(type, member)); })

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
    struct string name;
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



typedef struct script{
    char name[256];
    int  stages;
    double  order;
    struct script *next;
}script_t;

typedef struct lua_env{
    lua_State *L;
    script_t *scripts;
}lua_env_t;



struct sp_thread{
    record_t *current;
    record_t *record;
    record_t *record_tail;

    int64_t lines_num;
    int64_t records_num;
    int64_t records_num_nomatch;
    int64_t records_num_errmatch;
    int64_t records_num_droped;

    iterm_t *logs;

    lua_env_t lua_env;

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

    lua_env_t lua_env;

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
    int option_log_level;
    int option_src_type;
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

int sp_lua_tolstring(lua_State *L, int index, string_t *ss);
int sp_stage_lua_call(lua_State *L, const char *name);
int sp_stage_lua_callx(lua_State *L, const char *name, int nargs, int nresults);

lua_env_t splua_init(struct spectrum *sp, void *data);

#endif





