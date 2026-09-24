#include "smallrag/logging.h"
#include <errno.h>
#include <stdnoreturn.h>
#include <stdarg.h>

static void print_log_meta(char *level_prefix)
{
    fprintf(smallragLOG_OUT, "[%s] %s> %s:%s:%d: ", level_prefix, __TIME__, __FILE__, __func__, __LINE__);
}

static void priv_log(char *level_prefix, char *msg, va_list args)
{
    print_log_meta(level_prefix);
    vfprintf(smallragLOG_OUT, msg, args);
    fprintf(smallragLOG_OUT, "\n");
}

#define privCALL_LOG_INTERNAL(level_prefix) \
    va_list args; \
    va_start(args, msg); \
    priv_log(#level_prefix, msg, args);

void smallrag_logd(char *msg, ...)
{
#if ((smallragLOG_LEVEL) >= (smallragLOG_LEVEL_DEBUG))
privCALL_LOG_INTERNAL(DEBUG)
#endif
}

void smallrag_logi(char *msg, ...)
{
#if ((smallragLOG_LEVEL) >= (smallragLOG_LEVEL_INFO))
privCALL_LOG_INTERNAL(INFO)
#endif
}

void smallrag_logw(char *msg, ...)
{
#if ((smallragLOG_LEVEL) >= (smallragLOG_LEVEL_WARN))
privCALL_LOG_INTERNAL(WARN)
#endif
}

void smallrag_loge(char *msg, ...) 
{
#if ((smallragLOG_LEVEL) >= (smallragLOG_LEVEL_ERR))
privCALL_LOG_INTERNAL(ERR)
#endif
}


