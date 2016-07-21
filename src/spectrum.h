/**
 *   author       :   丁雪峰
 *   time         :   2016-07-21 05:50:53
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __SPECTRUM_H__
#define __SPECTRUM_H__

#define Malloc malloc
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



struct item_string{
    struct string s;
    struct item_string *next;
};

struct item_number{
    struct number n;
    struct item_number *next;
};

struct item_array{
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
};

struct format_var{
    struct string name;
    int type;
    char endflag;
    int space;
};

struct format{
    int start_skip;
    struct format_var vars[];
};


#endif


