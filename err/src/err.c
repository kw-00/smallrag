#include "smallrag/err.h"
#include "smallrag/logging.h"
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef smallragERR_BT_LIMIT
#define smallragERR_BT_LIMIT 256
#endif
#ifndef smallragERR_MSG_LIMIT
#define smallragERR_MSG_LIMIT 256
#endif

enum priv_err_entry_type {
    privERR_ERR,
    privERR_ERRNO
};

struct priv_err_entry {
    enum priv_err_entry_type type;
    union {
        smallrag_err_t err;
        int errn;
    };
    char msg[smallragERR_MSG_LIMIT];
    char *time;
    char *file;
    const char *func;
    int line;
};

static _Thread_local struct priv_err_entry err_stack[smallragERR_BT_LIMIT];
static _Thread_local  struct priv_err_entry *err_stack_ptr = NULL;

#define privPUSH_ERR_STACK_PTR() \
    do { \
        if (err_stack_ptr == NULL) err_stack_ptr = err_stack; \
        if (err_stack_ptr == &err_stack[smallragERR_BT_LIMIT - 1]) { \
            smallrag_loge("Cannot push error — error stack would overflow"); \
        } \
        err_stack_ptr++; \
    } while (false)
    
#define privINIT_ERR_ENTRY_META \
    .time = __TIME__, \
    .file = __FILE__, \
    .func = __func__, \
    .line = __LINE__,


void smallrag_pusherr(smallrag_err_t err)
{
    privPUSH_ERR_STACK_PTR(); 
    *err_stack_ptr = (struct priv_err_entry){
        .type = privERR_ERR,
        .err = err,
        privINIT_ERR_ENTRY_META
    };
    err_stack_ptr->msg[0] = '\0';
}

void smallrag_pusherrmsg(smallrag_err_t err, char *msg)
{
    privPUSH_ERR_STACK_PTR();
    *err_stack_ptr = (struct priv_err_entry){
        .type = privERR_ERR,
        .err = err,
        privINIT_ERR_ENTRY_META
    }; 
    const size_t msg_len = strlen(msg);
    memcpy(err_stack_ptr->msg, msg, MIN(smallragERR_MSG_LIMIT, msg_len));
    err_stack_ptr->msg[smallragERR_MSG_LIMIT - 1] = '\0';
}

void smallrag_pusherrstd()
{
    privPUSH_ERR_STACK_PTR();
    *err_stack_ptr = (struct priv_err_entry){
        .type = privERR_ERRNO,
        .errn = errno,
        privINIT_ERR_ENTRY_META
    };
}

#undef privCHECK_STACK_HAS_SPACE
#undef privINIT_ERR_ENTRY_META
smallrag_err_t smallrag_geterr()
{
    if (err_stack_ptr == err_stack) return NULL;
    return err_stack_ptr->err;
}

void smallrag_clearerr()
{
    err_stack_ptr = err_stack;
}

noreturn void smallrag_throw()
{
    struct priv_err_entry *cur = err_stack + 1;
    fputs("Traceback (most recent call last):\n", smallragERR_OUT);
    while (cur++ < err_stack_ptr) {
        fprintf(smallragERR_OUT, "%s> %s:%s:%d", __TIME__, __FILE__, __func__, __LINE__);
        if (cur->err != NULL) {
            if (cur->type == privERR_ERR) {
                fprintf(smallragERR_OUT, ": Exception occurred: %s\n", (char *)(cur->err));
            } else {
                fprintf(smallragERR_OUT, ": Exception occurred (errno %d): %s\n", cur->errn, strerror(cur->errn));
            }

        } else {
            fprintf(smallragERR_OUT, "\n");
        }
    }
    exit(1);

}
noreturn void smallrag_throwstd() 
{
    const int errno_snap = errno;
    fprintf(smallragERR_OUT, "%s> %s:%s:%d: ", __TIME__, __FILE__, __func__, __LINE__);
    fprintf(smallragERR_OUT, "Standard exception occurred: %s\n", strerror(errno_snap));
    exit(1);
}

