#ifndef __SOCKS5__H
#define __SOCKS5__H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <list>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>


#include "log.h"
#include "socks5_util.h"
#include "rkfifo.h"
using namespace std;

#define DELETE_P(p)  do{if(p!=NULL){delete p;p=NULL;}}while(0);

#define ENABLE_IPV6	1

#define    COMMTYPE_UNKNOW 0
#define	   COMMTYPE_WITH_CLIENT 1
#define    COMMTYPE_WITH_SERVICE 2


#define CMD_CONNECT 1
#define CMD_BIND	2
#define	CMD_UDP		3

#define IP_TYPE_V4		1
#define IP_TYPE_DOMIN	3
#define IP_TYPE_V6		4

#define    COMMERR_OK 		0
#define    COMMERR_READ 	1
#define    COMMERR_WRITE 	2
#define    COMMERR_CLOSE 	3
#define    CW_NULL 			4
#define    CR_NULL 			5
#define    CC_NULL 			6

#define DATA_BUF_SIZE (16*1024)

class socks5comm
{
    public:

    socks5comm(void *cl,int t):
		w_fifo(8*DATA_BUF_SIZE,1)
    {
    	channel = cl;
        fd = 0;
        type = t;

		err_code = COMMERR_OK;
        bzero(&ipv4addr,sizeof(ipv4addr));
		bzero(&ipv4addr,sizeof(ipv6addr));
		af = 0;
    }
    ~socks5comm()
    {

    }

	void set_err(uint8_t code)
	{
		err_code = code;
	}
		
    void *channel;
    int fd;//0:not_init 1:init_ok  -1:err
    int type;//0:client_fd  1:server_fd 2:listen_fd
    int af;//
    struct sockaddr_in  ipv4addr;
	struct sockaddr_in6 ipv6addr; 

	uint8_t err_code;
	struct epoll_event ev;
	rkfifo w_fifo;
	
};

extern uint16_t g_cc_num;
extern uint16_t g_cc_id;
class socks5channel
{

    public:
		socks5channel()
		{
			lastActTime = GetSysBootSeconds();			
			used_to_client = NULL;
			used_to_service = NULL;	
			EA_method = 0;//Encryption authentication
			bzero(cc_user,sizeof(cc_user));
			bzero(cc_passwd,sizeof(cc_passwd));
			id = (++g_cc_id)&0xffff;
			LOG_DBG("cc_num=%d,open channel(%u)\n",++g_cc_num,id);
		}
		~socks5channel()
		{
			geterr();		
			LOG_DBG("cc_num=%d,close channel(%u)c=%u,s=%u by [%04x]\n",--g_cc_num,id,used_to_client->fd,used_to_service->fd,errcode);
			colseAllcomm();
			DELETE_P(used_to_client);
			DELETE_P(used_to_service);
		}

		int init(char method,char * user=NULL,char *passwd=NULL)
		{
			EA_method = method;
			if(user!=NULL)strncpy(cc_user,user,255);
			if(passwd!=NULL)strncpy(cc_passwd,passwd,255);
				
			used_to_client = new socks5comm(this,COMMTYPE_WITH_CLIENT);
			if(used_to_client==NULL)return -1;
			used_to_service = new socks5comm(this,COMMTYPE_WITH_SERVICE);
			if(used_to_service==NULL)return -1;
			return 0;
		}

		

		void colseAllcomm()
		{
			if(used_to_client)
			{	
				if(used_to_client->fd>0)
				{
					close(used_to_client->fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, used_to_client->fd, NULL);
					used_to_client->fd = 0;
				}
			}

			if(used_to_service)
			{	
				if(used_to_service->fd>0)
				{
					close(used_to_service->fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, used_to_service->fd, NULL);
					used_to_service->fd = 0;
				}
			}
		}

		uint32_t geterr()
		{
			
			if(used_to_client)
			{	
				errcode = used_to_client->err_code;			
			}
			if(used_to_service)
			{	
				errcode |= used_to_service->err_code << 8;			
			}
			return errcode;
		}

		socks5comm	*used_to_client;
		socks5comm	*used_to_service;

		char cc_user[256];//len(1~255)
		char cc_passwd[256];//len(1~255)

		uint32_t lastActTime;
		int epollfd;
		char EA_method;//0
		uint16_t  id;
		uint16_t errcode;

};


class socks5Service
{
	public:
		socks5Service();
		~socks5Service();

		int init(uint16_t port,char method,int flag);	
		void setUserPass(char * user,char *passwd)
		{
			if(user!=NULL)strncpy(ss_user,user,255);
			if(passwd!=NULL)strncpy(ss_passwd,passwd,255);	
		}
		int run();

		friend void* thread_main( void * param );
		friend void* thread_watch( void * param );
		int new_thread(socks5channel    *cc);
		void getNetBytes(uint32_t &u,uint32_t &d)
		{
			u = upBytes;
			d = downBytes;
		}
	private:
		int handle_events(struct epoll_event *events, int num);
		int do_accpet();
		int do_read(struct epoll_event *ev);
		int do_err(struct epoll_event *ev);
		int do_write( struct epoll_event *ev);
		int writehandler(socks5comm *c_w,int epollfd);
		int readhandler(socks5comm *c_r,rkfifo &fifo);

		uint16_t listen_port;
		int listenfd;
		int epollfd;

		char EA_method;//0

		int events_max_num;
		int fdsize;

		uint32_t upBytes;
		uint32_t downBytes;


		//这不考虑多用户管理
		char ss_user[256];//len(1~255)
		char ss_passwd[256];//len(1~255)

		
};

#endif
