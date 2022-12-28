#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_LEVELS_WARN  1
#define LOG_LEVELS_DEBUG 2
#define LOG_LEVELS_TH	 4


extern int log_levels;
#define LOG(l,message, ...) do{if(l <= log_levels)fprintf(stdout, message, ##__VA_ARGS__);fflush(stdout);}while(0);

//#define LOG_DBG(message, ...)  LOG(LOG_LEVELS_DEBUG,message,##__VA_ARGS__)
#define LOG_DBG(message, ...)  my_log(LOG_LEVELS_DEBUG,message,##__VA_ARGS__)
#define LOG_WAR(message, ...)  my_log(LOG_LEVELS_WARN,message,##__VA_ARGS__)

#define LOG_TH(message, ...)  my_log(LOG_LEVELS_TH,message,##__VA_ARGS__)

void set_log_level(int l);
void my_log(int l, const char * fmt, ...);
void my_log2(int l, const char * fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif






