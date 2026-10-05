#include <stdio.h>
#include <stdbool.h>

/* To set LOG_LEVEL for a file that includes this header, define LOG_LEVEL as LOG_LEVEL_INFO, LOG_LEVEL_ERROR,
 * LOG_LEVEL_WARNING, LOG_LEVEL_DEBUG or LOG_LEVEL_TRACE before including this header. 
 *
 * Let's say we want to see debug logs in some_header.h:
 *
 * // some_header.h
 * #define LOG_LEVEL LOG_LEVEL_DEBUG
 * #include "smallrag/logging.h"
 * // implementation
 *
 * Being a macro, LOG_LEVEL setting above only affects log level for the translation unit it is 
 * set for.
 *
 * Similarly, defining LOG_OUTPUT_STREAM decides where logs go.
 */


#ifndef LOG_LEVEL
// Change this to change global default log level
#define LOG_LEVEL LOG_LEVEL_WARNING
#endif

#ifndef LOG_OUTPUT_STREAM
// Change this to change global default log output stream
#define LOG_OUTPUT_STREAM stderr
#endif

#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2 
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5

#define LOG(label, message, ...) \
    do { \
        fprintf(LOG_OUTPUT_STREAM, "[%s] %s:%s:%d>", #label, __FILE__, __func__, __LINE__); \
        fprintf(LOG_OUTPUT_STREAM, " "message"\n", ##__VA_ARGS__); \
    } while (false)


#undef LOG_INFO 
#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(message, ...) LOG(INFO, message, ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

#undef LOG_ERROR 
#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(message, ...) LOG(ERROR, message, ##__VA_ARGS__)
#else
#define LOG_ERROR(message, ...)
#endif

#undef LOG_WARNING 
#if LOG_LEVEL >= LOG_LEVEL_WARNING
#define LOG_WARNING(message, ...) LOG(WARNING, message, ##__VA_ARGS__)
#else
#define LOG_WARNING(message, ...)
#endif

#undef LOG_DEBUG 
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(message, ...) LOG(DEBUG, message, ##__VA_ARGS__)
#else
#define LOG_DEBUG(message, ...)
#endif

#undef LOG_TRACE 
#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define LOG_TRACE(message, ...) LOG(TRACE, message, ##__VA_ARGS__)
#else
#define LOG_TRACE(message, ...)
#endif

