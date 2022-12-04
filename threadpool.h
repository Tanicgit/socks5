#ifndef __THREADPOOL__H
#define __THREADPOOL__H
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <sys/prctl.h>


#include "util.h"
#include "log.h"

using namespace std;


typedef void*(*THREAD_TASK)(void *);
typedef enum
{
	STS_NO_INIT=0,
	STS_IDLE,
	STS_BUSY,
	STS_EXIT
}THREAD_TASK_STATUS;


typedef void (*taskfree) (void  *);

class ThreadTask{

	public:
		ThreadTask();
		~ThreadTask();

		int Init(char *name);
		int ThreadExit();
		int run();
		int AddTask(THREAD_TASK f,void *p,taskfree tfree=NULL,void *tfree_parm=NULL);	

		bool ThreadIdleTooLong(uint32_t now,uint32_t timeout)
		{	
			LOG_TH("ThreadIdleTooLong now=%d,last=%d,%d\n",now,last_idle_tick,timeout);
			if(status == STS_IDLE)
			{
				if(now - last_idle_tick > timeout)
				{
					LOG_TH("del [%s]\n",th_name);
					return true;
				}
			}	
			return false;
		}
		

		friend void* ThreadTaskMain(void *parm);
	private:
	
	THREAD_TASK func;
	void *parm;
	THREAD_TASK_STATUS status;
	uint32_t last_idle_tick;

	taskfree tf;
	void *tf_parm;

	pthread_mutex_t thread_mutex;
	pthread_cond_t thread_crond;
	pthread_t tid;
	char th_name[16];
};

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	friend void *ThreadPoolMain(void *parm);
	int Init(int th_num=50,int min_num=50,int max_num=50);
	int addTask(THREAD_TASK f,void *p,taskfree tfree=NULL,void *tfree_parm=NULL);
	int run();
private:
	int th_id;//µÝÔö
	list<ThreadTask*> list_st;
	pthread_mutex_t list_st_mutex;
	pthread_t tid;
	char th_name[16];
	char minThNum;
	char maxThNum;
};
#endif
