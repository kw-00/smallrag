#ifndef SMALLRAG_LOGGING_H
#define SMALLRAG_LOGGING_H

#include <stdio.h>

#ifndef smallragLOG_OUT
#define smallragLOG_OUT stderr
#endif

enum smallrag_log_level {
    smallragLOG_LEVEL_ERR,
    smallragLOG_LEVEL_WARN,
    smallragLOG_LEVEL_INFO,
    smallragLOG_LEVEL_DEBUG
};

#ifndef smallragLOG_LEVEL
#define smallragLOG_LEVEL smallragLOG_LEVEL_INFO
#endif

void smallrag_logd(char *msg, ...);
void smallrag_logi(char *msg, ...);
void smallrag_logw(char *msg, ...);
void smallrag_loge(char *msg, ...);
#endif
