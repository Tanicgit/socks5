#include "socks5.h"
#include <list>
#include <signal.h>
#include <sys/stat.h>
#include "threadpool.h"
#include "log.h"
#include "tConfigFile.h"


using namespace std;
void daemonrun(void);
int main(int argc,char *argv[])
{
	int opt=0;
	string configfile="socks5.conf";
	char tmp[256]={0};

	set_log_level(LOG_LEVELS_WARN);
	
	const char *optstr="f:";
	while ( (opt = getopt( argc, argv, optstr)) != -1 )
	{
		switch (opt)
		{

			case 'f':
				configfile=optarg;
			break;

			default:
				break;
		}
	}

	tConfigFile  tcf(configfile);
	string value;
	
	int log_mask=0;
	if(tcf.get("main","log_mask",value))
	{
		log_mask = atoi(value.c_str());
		set_log_level(log_mask);
	}

	int enable_netspeed_dis=0;
	if(tcf.get("main","enable_netspeed_display",value))
	{
		enable_netspeed_dis = atoi(value.c_str());
	}
	
	int listenport=5038;
	if(tcf.get("main","listenport",value))
	{
		listenport = atoi(value.c_str());
	}

	int ea_method=0;
	if(tcf.get("main","ea_method",value))
	{
		ea_method = atoi(value.c_str());
	}

	int enable_daemon=1;
	if(tcf.get("main","enable_daemon",value))
	{
		enable_daemon = atoi(value.c_str());
	}

	string user="test";
	if(tcf.get("main","user",value))
	{
		user = value;
	}

	string passwd="test";
	if(tcf.get("main","passwd",value))
	{
		passwd = value;
	}

	string socks5_client_phy_dev="";
	if(tcf.get("main","socks5_client_phy_dev",value))
	{
		socks5_client_phy_dev = value;
	}

	
	string remote_service_phy_dev="";
	if(tcf.get("main","remote_service_phy_dev",value))
	{
		remote_service_phy_dev = value;
	}



	if(enable_daemon)daemonrun();
	
	socks5Service s5;
	char method = (char)ea_method;
	if(0==s5.init(listenport,socks5_client_phy_dev,remote_service_phy_dev,ea_method,enable_netspeed_dis))
	{
		s5.setUserPass(user.c_str(),passwd.c_str());
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


