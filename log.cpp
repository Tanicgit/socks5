#include "log.h"
#include <time.h>
#include <string.h>

int log_levels=LOG_LEVELS_WARN;
void set_log_level(int l)
{
    log_levels = l;
}



void my_log(int l, const char * fmt, ...)
{
    va_list ap;
    time_t t;
    struct tm * local;
    char s[1024],len;
    if(l <= log_levels)
    {
        t = time(NULL);
        local = localtime(&t);
        strftime(s,64,"[%Y-%m-%d %H:%M:%S] ",local);
        len = strlen(s);
        va_start(ap,fmt);
            vsprintf(s+len,fmt,ap);
            fprintf(stdout,s,ap);
        va_end(ap);
        fflush(stdout);
    }
}
