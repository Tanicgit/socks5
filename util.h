#ifndef __SOCKS5_UTIL__H
#define __SOCKS5_UTIL__H
#include <stdint.h>
#include <time.h>

#define DELETE_P(p)  do{if(p!=NULL){delete p;p=NULL;}}while(0);

uint32_t GetSysBootSeconds();
uint32_t GetSysBootMSeconds();
uint32_t GetSysBootUSeconds();


char* eatHeadTailSpaceChr(char *src);

#endif

