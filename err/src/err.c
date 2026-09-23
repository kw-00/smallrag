#include "smallrag/err.h"
#include "smallrag/logging.h"
#include <errno.h>

#ifndef smallragERR_BT_LIMIT
#define smallragERR_BT_LIMIT 256
#endif

struct priv_err_entry {
    smallrag_err_t err;
    char *time;
    char *file;
    char *func;
    int *line;
};



static _Thread_local struct priv_err_entry err_stack[smallragERR_BT_LIMIT];
static _Thread_local  struct priv_err_entry *err_stack_ptr = err_stack;

void smallrag_pusherr(smallrag_err_t err)
{
    if (err_stack_ptr == &err_stack[smallragERR_BT_LIMIT - 1]) {
        loge("Cannot push error — error stack would overflow");
    }
    err_stack_ptr++;
    *err_stack_ptr = (struct priv_err_entry){
        .err = err,
        .time = __TIME__,
        .file = __FILE__,
        .func = __func__,
        .line = __LINE__
    }; 
}

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
    fprintf(smallragERR_OUT, "%s> %s:%s:%s: ", __TIME__, __FILE__, __func__, __LINE__);
    if (err_stack_ptr > err_stack) {
        fprintf(smallragERR_OUT, "Exception occurred: %s\n", (char *)err_stack_ptr->err);
        err_stack_ptr--;
    }
    while (err_stack_ptr > err_stack) {
        fprintf(smallragERR_OUT, "Caused by exception: %s\n", (char *)err_stack_ptr->err);
        err_stack_ptr--;
    }
    exit(1);

}
noreturn void smallrag_throwstd() 
{
    int errno_snap = errno;
    fprintf(smallragERR_OUT, "%s> %s:%s:%s: ", __TIME__, __FILE__, __func__, __LINE__);
    fprintf(smallragERR_OUT, "Standard exception occurred: %s\n", strerror(errno_snap));
    exit(1);
}

