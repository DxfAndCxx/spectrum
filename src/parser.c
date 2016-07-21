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
    void *var;
    if (VAR_TYPE_STR == type)
    {
         var = Malloc(sizeof(struct item_string));
         if (NULL == record->string)
         {
             record->string = var;
         }
         else{
             record->string_tail->next = var;
         }
         record->string_tail = var;
         return var;
    }

    if (VAR_TYPE_NUM == type)
    {
         var = Malloc(sizeof(struct item_number));
         if (NULL == record->number)
         {
             record->number = var;
         }
         else{
             record->number_tail->next = var;
         }
         record->number_tail = var;
         return var;
    }

    if (VAR_TYPE_NUM_ARRAY == type)
    {
         var = Malloc(sizeof(struct item_array) + sizeof(struct number) * size);
         if (NULL == record->array)
         {
             record->array = var;
         }
         else{
             record->array_tail->next = var;
         }
         record->array_tail = var;
         return var;
    }
}

static const char * skip_varname(const char *c)
{
    while(*c)
    {
        ++c;
        if (*c >= 'a' && *c <= 'z') continue;
        if (*c >= 'A' && *c <= 'Z') continue;
        if (*c == '_') continue;

        return c;
    }
    return c;
}

struct format *compile(const char *fmt)
{
    const char *c;
    struct format *format;

    int count = 0;
    int size;

    c = fmt;
    while(*c)
    {
        if (*c == '$') ++count;
        ++c;
    }



    size = sizeof(*format) + sizeof(struct format_var) * count + 1;
    format = malloc(size);
    memset(format, 0, size);


    c = fmt;
    count = 0;
    while (*c)
    {
        if (*c == '$')
        {
            if (0 == count)
                format->start_skip = c - fmt;
            else
                format->vars[count - 1].space = c - format->vars[count - 1].name.s - format->vars[count - 1].name.l;

            format->vars[count].name.s = (char *)c + 1;
            c = skip_varname(c);

            format->vars[count].name.l = c - format->vars[count].name.s;
            format->vars[count].type = VAR_TYPE_STR;

            if (*c)
                format->vars[count].endflag = *c;
            else
                format->vars[count].endflag = '\n';

            ++count;
            continue;
        }
        ++c;
    }

    return format;

}


#if 1
int main()
{
    struct format *fmt;
    struct format_var *var;

    const char *ngx_log = "'$remote_addr - $remote_user [$time_local] '"
    "'\"$request_method $scheme://$http_host$request_uri $server_protocol\" '"
    "'$status $body_bytes_sent \"$http_referer\" \"$http_user_agent\" '"
    "'\"$sent_http_content_type\" $domain_type $http_x_request_id $request_time '"
    "'\"$upstream_http_cache_control\" \"$upstream_http_x_source\" '"
    "'\"$cache_status\" \"$upstats_dynamic\" \"$upstream_addr\" \"$upstream_status\" '"
    "'\"$upstream_response_time\" \"$upstream_response_length\" $upstream_http_age '"
    "'$uptype $upage \"$sent_http_via\" $first_byte_time '"
    "'\"$upstream_http_x_slice_size\"'";

    fmt = compile(ngx_log);

    printf("format start_skip: %d\n", fmt->start_skip);
    var = fmt->vars;

    while(var->name.s)
    {
        printf("var: %-30.*s endswith: [%c] type: %d space: %d\n",
                var->name.l, var->name.s, var->endflag,
                var->type, var->space
                );
        ++var;

    }




}
#endif

