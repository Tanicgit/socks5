#include "util.h"
#include <string.h>
uint32_t GetSysBootSeconds()
{
    struct timespec ts;
    uint32_t   t = 0;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    t = ts.tv_sec;
    return t;
}
uint32_t GetSysBootMSeconds()
{
    struct timespec ts;
    uint32_t   t = 0;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    t = ts.tv_sec;
    t = t * 1000 + ts.tv_nsec / 1000000;
    return t;
}

uint32_t GetSysBootUSeconds()
{
    struct timespec ts;
    uint32_t   t = 0;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    t = ts.tv_sec;
    t = t * 1000000 + ts.tv_nsec / 1000;
    return t;
}



/*
É¾³ý×Ö·û´®Í·Î²µÄ¿Õ°××Ö·û,Ö±½ÓÌæ»»Îª \0
*/
#define IS_SPACE_CHR(a) ((a)=='\r' || (a)=='\n' || (a)=='\t' || (a)==' ')
char* eatHeadTailSpaceChr(char *src)
{
	char *dst = src;
	while(IS_SPACE_CHR(*dst))
	{
		if(*dst == 0)return dst;
		*dst = 0;
		dst++;
	}	

	int len = strlen(dst);
	char *tail = dst + len - 1;

	while(IS_SPACE_CHR(*tail))
	{
		*tail = 0;
		tail--;
	}
	return dst;
}


