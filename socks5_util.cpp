#include "socks5_util.h"
#include <string.h>
uint32_t GetSysBootSeconds()
{
    struct timespec ts;
    uint32_t   t = 0;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    t = ts.tv_sec;
    return t;
}


/*
ɾ���ַ���ͷβ�Ŀհ��ַ�,ֱ���滻Ϊ \0
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


