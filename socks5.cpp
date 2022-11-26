/*
https://www.rfc-editor.org/

rfc1928 SOCKS Protocol Version 5
rfc1929 Username/Password Authentication for SOCKS V5
RFC 1961 GSS-API Authentication Method for SOCKS Version 5
RFC 3089 A SOCKS-based IPv6/IPv4 Gateway Mechanism
*/


#include "socks5.h"

uint16_t g_cc_id=0;
uint16_t g_cc_num=0;


#define MAX_THREAD	256

int g_thread_num=0;

pthread_mutex_t cc_list_mut = PTHREAD_MUTEX_INITIALIZER;
list<socks5channel*> g_channel_list;


void cc_list_add(socks5channel *cc)
{
	pthread_mutex_lock(&cc_list_mut);
	g_channel_list.push_back(cc);
	pthread_mutex_unlock(&cc_list_mut);
}

 int setnonblock(int fd , int nonblock){
	 int flag = fcntl(fd,F_GETFL,0);
	 if(nonblock)
		 return fcntl(fd,F_SETFL,flag|O_NONBLOCK);
	 else
		 return fcntl(fd,F_SETFL,flag&~O_NONBLOCK);
 }
int setsocketTimeout(int fd,int s)
 {
 	struct timeval timeout = {s,0};
	return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
}


int readdata(int fd,uint8_t *r_buf,int len)
{

	if(0!=setsocketTimeout(fd,15))
	{
	    perror("setsocketTimeout ");
        close(fd);
        return -1;
	}
	
	int nread = read(fd ,r_buf,len);   	
    if (nread == -1)
    {
        perror("read error"); 
        return -1;
    }
    else if (nread == 0)
    { 
		LOG_DBG("close by client\n");
        return -1;
    }
    else
    {
        return nread;
    } 
}

int methodUserPawwsd(socks5channel *cc)
{
	int nread;
	uint8_t r_buf[510];
	uint8_t w_buf[2];
	uint8_t user[256]={0};
	uint8_t user_len=0;
	uint8_t passwd[256]={0};
	uint8_t passwd_len=0;
	uint8_t ret=0;
	nread = readdata(cc->used_to_client->fd,r_buf,256);
	
	if(nread<5)
	{
		ret = 1;
		goto exit_flag;
	}

	if(r_buf[0]!=1)
	{
		ret = 1;
		goto exit_flag;

	}

	user_len = r_buf[1];
	memcpy(user,r_buf+2,user_len);
	passwd_len = r_buf[2+user_len];
	memcpy(passwd,r_buf+2+user_len+1,passwd_len);

	if(user_len!=strlen(cc->cc_user))
	{
		ret = 1;
		goto exit_flag;
	}

	if(passwd_len!=strlen(cc->cc_passwd))
	{
		ret = 1;
		goto exit_flag;
	}

	if(0!=memcpy(cc->cc_user,user,user_len))
	{
		ret = 1;
		goto exit_flag;
	}
	
	if(0!=memcpy(cc->cc_passwd,passwd,passwd_len))
	{
		ret = 1;
		goto exit_flag;
	}

	exit_flag:
	w_buf[0] = 1;
	w_buf[1] = ret;
	write(cc->used_to_client->fd,w_buf,2);
	return (ret==0)?0:-1;
	 
}

int EA_Init(socks5channel *cc)
{
   int nread;
   uint8_t r_buf[256];
   uint8_t select_method=0XFF;
   
   nread = readdata(cc->used_to_client->fd,r_buf,256);
   if(nread<=0)
   {
	   return -1;
   }

   //1.加密认证方式协商
   if(r_buf[0] != 5)//版本5
   {
	   return -1;
   }
   
   uint8_t nmethods;
   nmethods = r_buf[1];
   if(nmethods+2 > nread)
   {
	   return -1;
   }

   uint8_t *methods = r_buf+2;
   for(int i=0;i<nmethods;i++)
   {
	   if(cc->EA_method == methods[i])
	   {
		   select_method = methods[i];	   
		   break;
	   }
   }
   u_char w_buf[2];
   w_buf[0] = 5;
   w_buf[1] = select_method;
   write(cc->used_to_client->fd,w_buf,2);

   if(select_method == 0)
   {
	   //none	  
   }
   else if(select_method == 1)
   {
		//GSSAPI
   }
   else if(select_method == 2)
   {
		//USER PASSWD
		return methodUserPawwsd(cc);
   }
   else
   {
		return -1;
   }

   return 0;
}

 int dns_func(const char *str_host, int *af,uint8_t addr[16])
 {
	if (!str_host) return -1;

	int h_err;
	char buffer[4096];
	char ip_str[128];
	int taf=0;

	struct hostent hbuf;
	struct hostent *hostEntry=NULL;
	gethostbyname_r(str_host,&hbuf,buffer,sizeof(buffer),&hostEntry,&h_err); 

	if(hostEntry == NULL)
	{
		LOG_WAR("gethostbyname_r(%s) %s\n",str_host,hstrerror(h_err));
		return -1;
	}

	//select the 1th ip
	*af = hbuf.h_addrtype;
	taf = *af;
	memcpy(addr,hbuf.h_addr_list[0],16);
	
	LOG_DBG("----------------------------------\n");
		LOG_DBG("h_name=%s\n",hbuf.h_name);
		LOG_DBG("h_addrtype=%d\n",hbuf.h_addrtype);
		LOG_DBG("h_length=%d\n",hbuf.h_length);

		for(int i=0;hbuf.h_aliases[i]>0;i++)
		{
			LOG_DBG("\th_aliases=%s\n",hbuf.h_aliases[i]);
		}
		
		for(int i=0;hbuf.h_addr_list[i]>0;i++)
		{		
			LOG_DBG("\th_addr_list=%s\n",inet_ntop(hbuf.h_addrtype,(struct in_addr*)hbuf.h_addr_list[i],ip_str,sizeof(ip_str)));			
		}	
	LOG_DBG("----------------------------------\n");
	#if 0
		LOG_DBG("h_name=%s\n",hostEntry->h_name);
		LOG_DBG("h_addrtype=%d\n",hostEntry->h_addrtype);
		LOG_DBG("h_length=%d\n",hostEntry->h_length);

		for(int i=0;hostEntry->h_aliases[i]>0;i++)
		{
			LOG_DBG("\th_aliases=%s\n",hostEntry->h_aliases[i]);
		}
		
		for(int i=0;hostEntry->h_addr_list[i]>0;i++)
		{
			LOG_DBG("\th_addr_list=%s\n",inet_ntop(hostEntry->h_addrtype,(struct in_addr*)hostEntry->h_addr_list[i],ip_str,sizeof(ip_str)));
		}	

	LOG_DBG("----------------------------------\n");
	#endif
	return 0;
 }


int cmd_connet_pro(socks5channel *cc,uint8_t atype,uint8_t *r_buf)
{
	int af=0;
	int ret;
	int ip_addr_len = 0;
	char errmsg[256];
	uint8_t addr[16];
	char ip_str[64];//AAAA:BBBC:CCCC:DDDD:9999:8888:7777:6666
	uint8_t w_buf[256];
	uint8_t w_len=0;

	if(atype==IP_TYPE_V4)
	{	
		ip_addr_len = 4;

		af = AF_INET;
		cc->used_to_service->af = AF_INET;
		cc->used_to_service->ipv4addr.sin_family = AF_INET;
		cc->used_to_service->ipv4addr.sin_addr.s_addr = *(in_addr_t *)(r_buf +4);
		uint8_t *p = r_buf + 4 + ip_addr_len;
		cc->used_to_service->ipv4addr.sin_port = htons(p[0]<<8 | p[1]);
	}
	else if(atype==IP_TYPE_DOMIN)
	{
		char str_host[256]={0};		
		char str_host_len = r_buf[4];
		memcpy(str_host,r_buf+5,str_host_len);
		uint8_t ipaddr[16]={0};

		if(0==dns_func(str_host,&af,addr))
		{
			if(af == AF_INET)
			{
				cc->used_to_service->af = AF_INET;
				cc->used_to_service->ipv4addr.sin_family = AF_INET;		
				cc->used_to_service->ipv4addr.sin_addr.s_addr = *(in_addr_t *)(addr);
				uint8_t *p = r_buf + 5 + str_host_len;
				cc->used_to_service->ipv4addr.sin_port = htons(p[0]<<8 | p[1]);

			}
			else if(af == AF_INET6)
			{
				cc->used_to_service->af = AF_INET6;
				cc->used_to_service->ipv6addr.sin6_family = AF_INET6;												
				memcpy(&cc->used_to_service->ipv6addr.sin6_addr,addr,16);
				uint8_t *p = r_buf + 5 + str_host_len;
				cc->used_to_service->ipv6addr.sin6_port = htons(p[0]<<8 | p[1]);
			}
			else
			{
				return 4;
			}
		}
		else
		{
			return 4;
		}
	}
	#if ENABLE_IPV6
	else if(atype==IP_TYPE_V6)
	{	
		af = AF_INET6;
		ip_addr_len = 16;
		cc->used_to_service->af = AF_INET6;
		cc->used_to_service->ipv6addr.sin6_family = AF_INET6;
		memcpy(&cc->used_to_service->ipv6addr.sin6_addr,r_buf +4,16);
		uint8_t *p = r_buf + 4 + ip_addr_len;
		cc->used_to_service->ipv6addr.sin6_port = htons(p[0]<<8 | p[1]);
		return 8;		
	}
	#endif
	else
	{
		return 8;
	}
	
	int fd = socket(af,SOCK_STREAM,0);
	if(fd<0)
	{
		perror("[cmd connect] socket error:");
		return 1;
	}

	int syncnt = 4;
	ret = setsockopt(fd, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt));//1+2+4+8=15s超时
	if(ret == -1)
	{	
		sprintf(errmsg,"[cmd connect]setsockopt TCP_SYNCNT err ");
		perror(errmsg);	
		return 1;	
	}

	cc->used_to_service->fd = fd;
	if(af == AF_INET)
	{
		sprintf(errmsg,"[cmd connect] connect [%s]:%d",inet_ntop(AF_INET,&cc->used_to_service->ipv4addr.sin_addr,ip_str,sizeof(ip_str)),ntohs(cc->used_to_service->ipv4addr.sin_port));
		ret = connect(fd,(struct sockaddr *)&cc->used_to_service->ipv4addr,sizeof(struct sockaddr_in));
	}
	else if(af == AF_INET6)
	{
		sprintf(errmsg,"[cmd connect] connect [%s]:%d",inet_ntop(AF_INET6,&cc->used_to_service->ipv6addr.sin6_addr,ip_str,sizeof(ip_str)),ntohs(cc->used_to_service->ipv6addr.sin6_port));
		ret = connect(fd,(struct sockaddr *)&cc->used_to_service->ipv6addr,sizeof(struct sockaddr_in6));
	}
	if(ret == -1)
	{		
		perror(errmsg);	
		return 3;	
	}
	LOG_DBG("%s ok\n",errmsg);
	w_buf[0] = 5;
	w_buf[1] = 0;
	w_buf[2] = 0;
	if(af == AF_INET)
	{
		w_buf[3] = IP_TYPE_V4;
		memcpy(w_buf+4,&cc->used_to_service->ipv4addr.sin_addr,4);
		memcpy(w_buf+8,&cc->used_to_service->ipv4addr.sin_port,2);
		w_len = 10;
	}
	else
	{
		w_buf[3] = IP_TYPE_V6;
		memcpy(w_buf+4,&cc->used_to_service->ipv6addr.sin6_addr,16);
		memcpy(w_buf+20,&cc->used_to_service->ipv6addr.sin6_port,2);
		w_len = 22;
	}

	write(cc->used_to_client->fd,w_buf,w_len);
	

	setnonblock(cc->used_to_client->fd,1);
	cc->used_to_client->ev.events = EPOLLIN|EPOLLET;
	cc->used_to_client->ev.data.ptr = cc->used_to_client;
	epoll_ctl(cc->epollfd,EPOLL_CTL_ADD,cc->used_to_client->fd,&cc->used_to_client->ev);

	setnonblock(cc->used_to_service->fd,1);
	cc->used_to_service->ev.events = EPOLLIN|EPOLLET;
	cc->used_to_service->ev.data.ptr = cc->used_to_service;	
	epoll_ctl(cc->epollfd,EPOLL_CTL_ADD,cc->used_to_service->fd,&cc->used_to_service->ev);

	cc->lastActTime = GetSysBootSeconds();
	return 0;
}

/*
60s  timout
create a tcp-service ,close listen-fd when a client connect ok

note:V4-service can not accecpt V6-client
*/
int cmd_bind_pro(socks5channel *cc,uint8_t atype,uint8_t *r_buf)
{
	
	uint8_t w_buf[256];
	uint8_t w_len=0;
	int ret=0;

	w_buf[0] = 5;
	w_buf[1] = 0;
	w_buf[2] = 0;
	
	int af = cc->used_to_client->af;

	int fd = socket(af,SOCK_STREAM,0);
	if (fd == -1)
    {
        perror("[cmd bind] socket error:");
        return 1;
    }


	struct sockaddr_in  ipv4addr;
	struct sockaddr_in6  ipv6addr;

	bzero(&ipv4addr,sizeof(ipv4addr));
	bzero(&ipv6addr,sizeof(ipv6addr));
	if(af == AF_INET)
	{
		//V4服务端不能处理V6客户端 
		ipv4addr.sin_family = AF_INET;
		ipv4addr.sin_port = htons(0);
		ipv4addr.sin_addr.s_addr = 	inet_addr("0.0.0.0");
		
		w_buf[3] = IP_TYPE_V4;
		memcpy(w_buf+4,&ipv4addr.sin_addr.s_addr,4);					
		ret = bind(fd,(const sockaddr*)&ipv4addr, sizeof(struct sockaddr_in));

		uint16_t port = ntohs(ipv4addr.sin_port);
		w_buf[8] = port >>8;
		w_buf[9] = port &0xff;
		w_len = 10;
		LOG_DBG("sin_port=%d",ipv4addr.sin_port);
	}
	#if ENABLE_IPV6
	else if(af == AF_INET6)
	{
		//V6服务端可以处理V4和V6客户端
		ipv6addr.sin6_family = AF_INET6;
		ipv6addr.sin6_port = htons(0);
		memset(&ipv6addr.sin6_addr,0,16);
		w_buf[3] = IP_TYPE_V6;
		memcpy(w_buf+4,&ipv6addr.sin6_addr,16);
		ret = bind(fd,(const sockaddr*)&ipv6addr, sizeof(struct sockaddr_in6));

		uint16_t port = ntohs(ipv6addr.sin6_port);
		w_buf[20] = port >>8;
		w_buf[21] = port &0xff;
		w_len = 22;

		LOG_DBG("sin_port=%d",ipv6addr.sin6_port);
	}
	#endif
	else
	{	
		return 8;	
	}
	
	if ( ret== -1)
    {
        perror("[cmd bind] bind error:");
		close(fd);
        return 1;
    }

	
	if(0!=listen(fd, 1))
    {
        perror("[cmd bind] listen error:");
        close(fd);
        return 1;
    }
	
	//这里要返回给client第一次,主要是把监听端口告知 client
	write(cc->used_to_client->fd,w_buf,w_len);

	int clifd;
    struct sockaddr_in6 v6_cliaddr;
	struct sockaddr_in *v4_cliaddr = (struct sockaddr_in*)&v6_cliaddr;
	struct sockaddr* cliaddr = (struct sockaddr*)&v6_cliaddr;	
    socklen_t cliaddrlen=sizeof(v6_cliaddr);

	if(0!=setsocketTimeout(fd,60))
	{
	    perror("[cmd bind] set accept timeout failed");
        close(fd);
        return 1;
	}
	
    clifd = accept(fd, cliaddr, &cliaddrlen);
    if (clifd == -1)
    {
        perror("[cmd bind] accept error:");
        close(fd);
        return 1;
    }
    else
    {
    	close(fd);
				
		//ok 这里要返回给client第二次
		if(af == AF_INET)//V4 服务
		{
			if(cliaddr->sa_family == AF_INET)//ip协议栈应当拒绝   非V4客户端
			{
				w_buf[3] = IP_TYPE_V4;
				memcpy(w_buf+4,&v4_cliaddr->sin_addr.s_addr,4);
				memcpy(w_buf+8,&v4_cliaddr->sin_port,2);
				w_len = 10;
				cc->used_to_service->ipv4addr.sin_addr.s_addr = v4_cliaddr->sin_addr.s_addr;				
			}
			else
			{
				LOG_WAR("err:file=%s,line=%d\n",basename(__FILE__),__LINE__);
				return 2;
			}
		}
		else if(af == AF_INET6)//V6 服务,如果是V4客户端连接过来,IP协议栈应该将V4地址转换为V6
		{
			if(cliaddr->sa_family == AF_INET6)
			{
				w_buf[3] = IP_TYPE_V6;
				memcpy(w_buf+4,&v6_cliaddr.sin6_addr,16);
				memcpy(w_buf+20,&v6_cliaddr.sin6_port,2);
				w_len = 22;
				memcpy(&cc->used_to_service->ipv6addr.sin6_addr,&v6_cliaddr.sin6_addr,16);
			}
			else
			{
				LOG_WAR("err:file=%s,line=%d\n",basename(__FILE__),__LINE__);
				return 2;
			}
		}
		else
		{
			return 2;
		}
		write(cc->used_to_client->fd,w_buf,w_len);


		cc->used_to_service->af = cliaddr->sa_family;
		cc->used_to_service->fd = clifd;


		setnonblock(cc->used_to_client->fd,1);
		cc->used_to_client->ev.events = EPOLLIN|EPOLLET;
		cc->used_to_client->ev.data.ptr = cc->used_to_client;
		epoll_ctl(cc->epollfd,EPOLL_CTL_ADD,cc->used_to_client->fd,&cc->used_to_client->ev);

		setnonblock(cc->used_to_service->fd,1);
		cc->used_to_service->ev.events = EPOLLIN|EPOLLET;
		cc->used_to_service->ev.data.ptr = cc->used_to_service; 
		epoll_ctl(cc->epollfd,EPOLL_CTL_ADD,cc->used_to_service->fd,&cc->used_to_service->ev);

		cc->lastActTime = GetSysBootSeconds();
		return 0;	
	}	
}

int cmd_udp_pro(socks5channel *cc,uint8_t atype,uint8_t *r_buf)
{
	return 7;
}

int channel_init(socks5channel *cc)
{
	uint8_t r_buf[256]={0};
	uint8_t cmd=0;
	uint8_t atype=0;
	char w_buf[256];
	uint8_t ret = 0;	
	
	int nread = readdata(cc->used_to_client->fd,r_buf,256);
  	if(nread<0)
  	{
		return -1;
  	}
	
	if(r_buf[0] != 5)//版本5
	{
		ret = 1;
		goto err_exit;
	}

	cmd = r_buf[1];
	atype = r_buf[3];	

	if(cmd==CMD_CONNECT)
	{

		ret = cmd_connet_pro(cc,atype,r_buf);
		if(ret == 0)
		{
			cc->lastActTime = GetSysBootSeconds();
			cc_list_add(cc);
		}
		else
		{
			goto err_exit;
		}			
	}
	else if(cmd==CMD_BIND)
	{	
		ret = cmd_bind_pro(cc,atype,r_buf);
		if(ret == 0)
		{	
			cc->lastActTime = GetSysBootSeconds();
			cc_list_add(cc);
		}
		else
		{
			goto err_exit;
		}			
	}
	else if(cmd==CMD_UDP)
	{
		ret = cmd_udp_pro(cc,atype,r_buf);
		if(ret == 0)
		{
			cc->lastActTime = GetSysBootSeconds();
			cc_list_add(cc);
		}
		else
		{
			goto err_exit;
		}
	}
	else
	{		
		ret = 7;
		goto err_exit;	
	}
	return 0;

	err_exit:
	w_buf[0] = 5;
	w_buf[1] = ret;
	memcpy(w_buf+2,r_buf+2,nread-2);
	write(cc->used_to_client->fd,w_buf,nread);
	return -1;
}


void * thread_main(void *parm)
{
	//prctl(PR_SET_NAME, t->name);
	socks5channel *cc = (socks5channel*) parm;

	if(EA_Init(cc)!=0)
	{
		DELETE_P(cc);
		goto exit_flag;
	}

	if(channel_init(cc)!=0)
	{
		DELETE_P(cc);
		goto exit_flag;
	}
	else
	{
		goto exit_flag;
	}

	exit_flag:
	g_thread_num--;	
	LOG_DBG("g_thread_num=%d\n",g_thread_num);
	return NULL;
}

void* thread_watch( void * param )
{
	socks5Service *ss = (socks5Service*)param;
	uint32_t u0=0,u1=0;
	uint32_t d0=0,d1=0;
	while(1)
	{
		sleep(4);
		ss->getNetBytes(u1,d1);
		
		if(u1-u0<1024)
		{
			LOG_WAR("UP=%dB/s,DOWN=%dB/s\n",(u1-u0)>>2,(d1-d0)>>2);
		}
		else
		{
			LOG_WAR("UP=%dKB/s,DOWN=%dKB/s\n",(u1-u0)>>12,(d1-d0)>>12);
		}

		u0=u1;
		d0=d1;
	}
}



int socks5Service::new_thread(socks5channel    *cc)
{
	int ret = 0;
	
	if(g_thread_num > MAX_THREAD)
	{
		return -1;
	}
	
	pthread_t pid;
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);  
	pthread_attr_setstacksize(&thread_attr, 1024*1024);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if((ret=pthread_create(&pid, &thread_attr, thread_main, cc)) != 0 ) {
	
		pthread_attr_destroy(&thread_attr);		
		return ret;
	}
	pthread_attr_destroy(&thread_attr);

	g_thread_num++;
	LOG_DBG("g_thread_num=%d\n",g_thread_num);
	return 0;
}

socks5Service::socks5Service()
{
	events_max_num = 512;
	fdsize = 1024;
	listen_port = 1236;

	upBytes = 0;
	downBytes = 0;

	bzero(ss_user,sizeof(ss_user));
	bzero(ss_passwd,sizeof(ss_passwd));
}

socks5Service::~socks5Service()
{

}


int socks5Service::init(uint16_t port,char ea_method,int flag)
{
	int ret = 0;
	struct sockaddr_in servaddr;
	listen_port = port;

	EA_method = ea_method;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)
    {
        perror("socket error:");
        return -1;
    }
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, "0.0.0.0", &servaddr.sin_addr);
	servaddr.sin_port = htons(listen_port);
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        return -1;
    }
	LOG_WAR("bind ok\n");
	if(listen(listenfd,20)!=0)
	{
        perror("listen error: ");
        return -1;
    }

	LOG_WAR("listen ok on %d\n",listen_port);
	if(flag>0)
	{
		pthread_t pid;
		pthread_attr_t thread_attr;
		pthread_attr_init(&thread_attr);  
		pthread_attr_setstacksize(&thread_attr, 1024*1024);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
		if((ret=pthread_create(&pid, &thread_attr, thread_watch, this)) != 0 ) {
		
			pthread_attr_destroy(&thread_attr);		
			return ret;
		}
		pthread_attr_destroy(&thread_attr);
	}
	return 0;
}



int socks5Service::run()
{
    struct epoll_event events[events_max_num];
    int num;
    struct epoll_event ev;

    epollfd = epoll_create(fdsize);
    if(epollfd<0)
    {
    	perror("epoll_create error: ");
        exit(1);
    }	

	ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev)==-1)
    {
		perror("listenfd EPOLL_CTL_ADD error: ");
		exit(1);
	}
	while(1)
	{
		num = epoll_wait(epollfd, events, events_max_num, -1);
		if(num < 0)
		{
			perror("epoll_wait error");
			exit(1);
		}
		else if(num > 0)
		{
			if(handle_events(events,num))
			{
				if(g_channel_list.empty())
					continue;
				pthread_mutex_lock(&cc_list_mut);
				list<socks5channel*>::iterator itr;
				for(itr=g_channel_list.begin();itr!=g_channel_list.end();itr++)
				{
					socks5channel *cc = (*itr);
					uint32_t err_code = cc->geterr();
					if(err_code!=0)
					{
						itr=g_channel_list.erase(itr);	
						DELETE_P(cc);
					}
				}
				pthread_mutex_unlock(&cc_list_mut);				
			}
		}
		else
		{

		}
			

	}
}

int socks5Service::handle_events(struct epoll_event *events, int num)
{
    int i;
    int fd;
    int ret=0;
    for (i = 0; i < num; i++)
    {
        fd = events[i].data.fd;
		
        if ((fd == listenfd) && (events[i].events & EPOLLIN))
        {		
            ret |= do_accpet();           
        }          
        else if (events[i].events & EPOLLIN )
        { 	
           ret |= do_read(&events[i]);           
        }
        else if (events[i].events & EPOLLOUT )
        {		
           ret |= do_write(&events[i]);           
        }
        else
        {
           ret |= do_err(&events[i]); 
           LOG_DBG("other:events=0x%x",events[i].events);
        }       
    }
    return ret;
}

int socks5Service::do_accpet()
{
    int clifd,ret;
    struct sockaddr_in cliaddr;
    struct epoll_event ev;
    socklen_t cliaddrlen=sizeof(cliaddrlen);

	bzero(&cliaddr,sizeof(cliaddr));
    clifd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if (clifd == -1)
    {
        perror("accpet error");
		exit(-1);
    }
    else
    {
    	//new channel
		socks5channel *cc = new socks5channel();
		if(cc==NULL)
		{
			close(clifd);
			LOG_WAR("new socks5channel() error\n");
			return 0;
		}

		if(cc->init(EA_method,ss_user,ss_passwd)!=0)
		{
			close(clifd);
			LOG_WAR("socks5channel() init error\n");
			DELETE_P(cc);
			return 0;
		}	
    	cc->epollfd = epollfd;
		cc->lastActTime = GetSysBootSeconds();

		cc->used_to_client->af = AF_INET;	
		cc->used_to_client->ipv4addr = cliaddr;
		cc->used_to_client->fd = clifd;
		cc->used_to_client->type = COMMTYPE_WITH_CLIENT;
		if(0==new_thread(cc))
		{
			
		}
		else
		{
			close(clifd);
		}

    }
    return 0;
}

int socks5Service::writehandler(socks5comm *c_w,int epollfd)
{
	char errmsg[256];
	int nwrite=0;
	int all_w_len=0;
	uint8_t buff[DATA_BUF_SIZE];
	while(1)
	{
		int nwrite = c_w->w_fifo.rkfifo_out_peek(buff,DATA_BUF_SIZE);
		if(nwrite==0)
		{
			break;//fifo没数据
		}
		
		int w_len = write(c_w->fd,buff,nwrite);
		if(w_len<0)
		{
			if(EWOULDBLOCK == errno || EAGAIN == errno)
			{
				struct epoll_event ev;			
				ev.events = c_w->ev.events | EPOLLOUT;
				ev.data.ptr = c_w;				
				epoll_ctl(epollfd,EPOLL_CTL_MOD,c_w->fd,&ev);	
				//sprintf(errmsg,"line=%d,write",__LINE__);
				//perror(errmsg); 
				break;//发送缓冲满
			}
			else
			{
				sprintf(errmsg,"line=%d,write",__LINE__);
				perror(errmsg); 
				c_w->set_err(COMMERR_WRITE);
				return -1;
			}
		}
		else
		{
			all_w_len += w_len;
			c_w->w_fifo.rkfifo_out_peek2(w_len);//write ok,remove date from fifo
			if(nwrite<w_len)//kernel write buff full
			{
				break;
			}
		}	
	}
	upBytes += all_w_len;
	return all_w_len;
}

int socks5Service::readhandler(socks5comm *c_r,rkfifo &fifo)
{
    int r_len=0;
	int to_r_len=0;
	uint32_t fifo_remain_len=0;
	char errmsg[256];
	uint8_t buff[DATA_BUF_SIZE];
	int all_r_len=0;
	while(1)
	{
		fifo_remain_len = fifo.kfifo_unused();
		if(fifo_remain_len>DATA_BUF_SIZE)
		{
			to_r_len = DATA_BUF_SIZE;
		}
		else if (fifo_remain_len==0)
		{
			break ;//fifo满
		}
		else
		{
			to_r_len = fifo_remain_len;
		}

		r_len = read(c_r->fd,buff,to_r_len);
		if (r_len == -1)
	    {
	    	if(errno == EAGAIN || errno == EWOULDBLOCK)
	    	{
	    		sprintf(errmsg,"line=%d,read",__LINE__);
				perror(errmsg); 
				break ;//读缓冲空
	    	}
			else
			{
				sprintf(errmsg,"line=%d,read",__LINE__);
				perror(errmsg); 
				c_r->set_err(COMMERR_READ);
				return -1;
			}   
	    }
	    else if (r_len == 0)
	    { 
	    	//sprintf(errmsg,"line=%d,read",__LINE__);
			//perror(errmsg); 
			c_r->set_err(COMMERR_CLOSE);
	        return -1;
	    }
	    else
	    {	
	    	all_r_len += r_len;
			fifo.rkfifo_in(buff,r_len);	
			if(to_r_len>r_len)//kernel read buf empty
			{
				break;
			}
	    } 		
	}
	downBytes += all_r_len;
	return all_r_len;
}


int socks5Service::do_read(struct epoll_event *ev)
{
    int nread=0;
    socks5comm *c_r=NULL;
	socks5comm *c_w=NULL;
	socks5channel *cc =NULL;

    c_r = (socks5comm*)ev->data.ptr;
	if(c_r==NULL)
	{
		return 1;
	}
	cc = (socks5channel *)c_r->channel;
	if(cc ==NULL)
	{
		c_r->set_err(CC_NULL);
		return 1;
	}
	if(c_r->type == COMMTYPE_WITH_CLIENT)
	{
		c_w = cc->used_to_service;
	}
	else
	{
		c_w = cc->used_to_client;
	}

	if(c_w==NULL)
	{
		c_r->set_err(CW_NULL);
		return 1;
	}

	nread = readhandler(c_r,c_w->w_fifo);
    if (nread == -1)
    {
    	return 1; 
    }
    else
    {				
		if(writehandler(c_w,cc->epollfd)<0) 
		{
			return 1;
		}
    } 
	return 0;
}

int socks5Service::do_err( struct epoll_event *ev)
{
	socks5comm *c_r=NULL;
	c_r = (socks5comm*)ev->data.ptr;
	c_r->set_err(COMMERR_CLOSE);
	return 0;
}

int socks5Service::do_write(struct epoll_event *ev)
{
	socks5comm *c_w=NULL;
	int ret = 0;
	
	c_w = (socks5comm*)ev->data.ptr;
	if(c_w!=NULL)
	{
		socks5channel *cc = (socks5channel *)c_w->channel;
		ret=writehandler(c_w,cc->epollfd);
		if(ret<0) 
		{
			return 1;
		}
		else
		{
			if(c_w->w_fifo.kfifo_unused()==0)//fifo full
			{			
				epoll_ctl(epollfd,EPOLL_CTL_MOD,c_w->fd,&c_w->ev);		
			}
		}
			
	}
	else
	{
		c_w->set_err(COMMERR_WRITE);
		return 1;		
	}
	return 0;
}




