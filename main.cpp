#include "socks5.h"
#include <list>
#include<signal.h>
#include <sys/stat.h>


using namespace std;
void daemonrun(void);

int main(int argc,char *argv[])
{
	int opt=0;
	int back=0;
	int flag=0;
	int ea_method = 2;
	int port = 5038;
	char user[256]="test";
	char passwd[256]="test";
	char configfile[1024]="socks5.conf";

	set_log_level(LOG_LEVELS_WARN);
	
	const char *optstr="f:";
	while ( (opt = getopt( argc, argv, optstr)) != -1 )
	{
		switch (opt)
		{

			case 'f':
				strcpy(configfile,optarg);
				printf("configfile=%s\n",configfile);
			break;

			default:
				printf("f:set configfile\n");
				break;
		}
	}


	FILE *fd = fopen(configfile,"r");
	if(fd!=NULL)
	{
		char line[1024];
		while(fgets(line,1023,fd )!=NULL)
		{
			line[1023] = 0;
			
			if (line[0] == '#')
			{
				continue;
			}
			
			char *key=eatHeadTailSpaceChr(line);
			char *value=strchr(key+1,'=');

			if(value == NULL)
			{
				continue;
			}
			value[0] = 0;
			value++;

			key = eatHeadTailSpaceChr(key);
			value = eatHeadTailSpaceChr(value);

			printf("key=%s,value=%s,\n",key,value);
			if(0==strcasecmp(key,"log_level"))
			{
				if(0==strcasecmp(value,"debug"))
				{
					set_log_level(LOG_LEVELS_DEBUG);				
				}
				else if(0==strcasecmp(value,"warn"))
				{
					set_log_level(LOG_LEVELS_WARN);
				}
			}
			else if(0==strcasecmp(key,"disp_net_speed"))
			{
				if(0==strcasecmp(value,"yes"))
				{
					flag=1;
				}
			}
			else if(0==strcasecmp(key,"listenport"))
			{			
				port = atoi(value);
			}
			else if(0==strcasecmp(key,"ea_method"))
			{			
				ea_method = atoi(value);// 0 1 2
			}
			else if(0==strcasecmp(key,"enable_daemon"))
			{			
				
				if(0==strcasecmp(value,"yes"))
				{
					back = 1;
				}
			}
			else if(0==strcasecmp(key,"user"))
			{						
				strncpy(user,value,255);
				user[255]=0;
			}
			else if(0==strcasecmp(key,"passwd"))
			{						
				strncpy(passwd,value,255);
				passwd[255]=0;
			}
			
		}
		fclose(fd);
	}
	


	if(back)daemonrun();
	
	socks5Service s5;
	char method = (char)ea_method;
	if(0==s5.init(port,method,flag))
	{
		s5.setUserPass(user,passwd);
		s5.run();
	}
	return -1;
}


void daemonrun(void)
{
		int i;
		int fd0;
		pid_t pid;
		struct sigaction sa;
		
		umask(0);
		
		pid = fork();
		if(pid < 0){
				perror("fork error!\n");
				exit(1);
		}else if(pid > 0){
				exit(0);
		}
		
		setsid(); 
		
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		
		if(sigaction(SIGCHLD, &sa, NULL) < 0){
				return ;
		}
		
		if(chdir("/") < 0){
				printf("chlid dir error!\n");
				return;
		}
		
		close(0);
		fd0 = open("/dev/null", O_RDWR);
		dup2(fd0, 1);
		dup2(fd0, 2);
}


