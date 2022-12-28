#include "log.h"
#include <time.h>
#include <string.h>
#include <sys/prctl.h>


int log_levels=LOG_LEVELS_WARN;
static struct timespec g_ts;
void set_log_level(int l)
{
    log_levels = l;
	clock_gettime( CLOCK_MONOTONIC, &g_ts );
}



void my_log(int l, const char * fmt, ...)
{
    va_list ap;
    time_t t;
	char name[16]={0};
    struct tm * local;
    char s[1024],len;
    if(l & log_levels)
    {
    	prctl(PR_GET_NAME, name);
        t = time(NULL);
        local = localtime(&t);
        len=strftime(s,64,"[%Y-%m-%d %H:%M:%S] ",local);
		len += sprintf(s+len,"[%s]",name);
        va_start(ap,fmt);
            vsprintf(s+len,fmt,ap);
            fprintf(stdout,s,ap);
        va_end(ap);
        fflush(stdout);
    }
}

void my_log2(int l, const char * fmt, ...)
{
    va_list ap;
    time_t t;
    struct tm * local;
	char name[16]={0};
    char s[1024],len;
    struct timespec ts;
    if(l & log_levels)
    {
    	prctl(PR_GET_NAME, name);
		clock_gettime( CLOCK_MONOTONIC, &ts );
		sprintf(s,"[%u %s]",ts.tv_sec-g_ts.tv_sec,name);
		
        len = strlen(s);
        va_start(ap,fmt);
            vsprintf(s+len,fmt,ap);
            fprintf(stdout,s,ap);
        va_end(ap);
        fflush(stdout);
    }
}

