#include "threadpool.h"
#include "string.h"



void *ThreadPoolMain(void *parm)
{
	prctl(PR_SET_NAME, "TP_MAGE");
	ThreadPool * tp =(ThreadPool*)parm;
	while(1)
	{
		sleep(60);
		tp->run();
	}
}

ThreadPool::ThreadPool()
{
	tid = 0;
	th_id = 0;
	minThNum = 50;
	maxThNum = 50;
	pthread_mutex_init(&list_st_mutex,NULL);	
}

ThreadPool::~ThreadPool()
{
	LOG_TH("~ThreadPool start\n");
	pthread_mutex_lock(&list_st_mutex); 
	if(!list_st.empty())
	{
		list<ThreadTask*>::iterator itr;
		for(itr=list_st.begin();itr!=list_st.end();)
		{
			ThreadTask *st = (*itr);			
			itr = list_st.erase(itr);
			DELETE_P(st);		
		}
	}
	pthread_mutex_unlock(&list_st_mutex);
	LOG_TH("~ThreadPool end\n");
}

int ThreadPool::run()
{
	uint32_t now = GetSysBootSeconds();
	
	pthread_mutex_lock(&list_st_mutex);
	list<ThreadTask*>::iterator itr;
	if(list_st.size()>minThNum)//少于20个就不释放了
	{
		for(itr=list_st.begin();itr!=list_st.end();)
		{
			ThreadTask *st = (*itr);	
			if(st->ThreadIdleTooLong(now,60)) //60s
			{
				itr = list_st.erase(itr);
				DELETE_P(st);
			}
			else
			{
				itr ++;
			}
			if(list_st.size()<=minThNum)
			{
				break;
			}
		}
	}
	LOG_TH("Thread NUM =%d\n",list_st.size());
	pthread_mutex_unlock(&list_st_mutex);
	return 0;
}

int ThreadPool::addTask(THREAD_TASK f,void *p,taskfree tfree,void *tfree_parm)
{
	ThreadTask *st=NULL;

	if(f == NULL)
		return -1;

	int ret = pthread_mutex_trylock(&list_st_mutex);
	if(ret == 0)
	{
		if(!list_st.empty())
		{
			list<ThreadTask*>::iterator itr;
			for(itr=list_st.begin();itr!=list_st.end() ;itr++)
			{
				st = (*itr);		
				if(0==st->AddTask( f, p,tfree,tfree_parm)) 
				{
					pthread_mutex_unlock(&list_st_mutex);	
					return 0;
				}
			}
		}

		if(list_st.size()<maxThNum)
		{
			st = new ThreadTask();
			if(st!=NULL)
			{
				list_st.push_back(st);
				char name[16];
				sprintf(name,"TH%04d",th_id++);
				LOG_TH("new [%s]\n",name);
				if(th_id>9999)th_id=0;
				st->Init(name);
				if(st->AddTask( f, p,tfree,tfree_parm)) 
				{
					pthread_mutex_unlock(&list_st_mutex);
					return 0;
				}
			}
		}
		pthread_mutex_unlock(&list_st_mutex);
	}	
	return -2;
}

int ThreadPool::Init(int th_num,int min_num,int max_num)
{
	int ret = 0;
	char name[16];
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);  
	pthread_attr_setstacksize(&thread_attr, 128*1024);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if((ret=pthread_create(&tid, &thread_attr, ThreadPoolMain, this)) != 0 ) {
		printf("pthread_create err %d\n",ret);
		pthread_attr_destroy(&thread_attr); 	
		return ret;
	}

	pthread_attr_destroy(&thread_attr); 	

	minThNum = min_num;
	maxThNum = max_num;

	for(int i=0;i<th_num;i++)
	{			
		ThreadTask *st = new ThreadTask();
		if(st!=NULL)
		{
			list_st.push_back(st);
			sprintf(name,"TH%04d",th_id++);
			LOG_TH("new [%s]\n",name);
			if(th_id>9999)th_id=0;		
			st->Init(name);
		}
	}
	return list_st.size();
}



/*########################################################################################*/

//void pthread_cleanup_push(void (*routine) (void  *),  void *arg)
//void pthread_cleanup_pop(int execute)
void *ThreadTaskMain(void *parm)
{
	ThreadTask * st=(ThreadTask*)parm;

	prctl(PR_SET_NAME, st->th_name);
	while(1)
	{
		if(0!=st->run())
		{
			break;
		}	
		pthread_testcancel();
	}
	LOG_TH("exit\n");
	return NULL;
}


ThreadTask::ThreadTask()
{
	status = STS_NO_INIT;
	func = NULL;
	parm = NULL;
	last_idle_tick = 0;
}

ThreadTask::~ThreadTask()
{
	int ret = 0;
	LOG_TH("[%s] exit\n",th_name);
	if(status!=STS_EXIT)
	{
		ThreadExit();
	}

	//todo 检查是否锁住,
	ret = pthread_mutex_trylock(&thread_mutex);
	if(ret == EBUSY || ret == 0)
	{
		//LOG_TH("pthread_mutex_trylock ret=%d\n",ret);
		pthread_mutex_unlock(&thread_mutex);
	}
	ret = pthread_mutex_destroy(&thread_mutex);
	//LOG_TH("pthread_mutex_destroy ret=%d\n",ret);

	ret = pthread_cond_signal(&thread_crond);
	//LOG_TH("pthread_cond_signal ret=%d\n",ret);
	ret = pthread_cond_destroy(&thread_crond);
	//LOG_TH("pthread_cond_destroy ret=%d\n",ret);
}

int ThreadTask::Init(char *name)
{
	int ret = 0;
	ret = pthread_mutex_init(&thread_mutex,NULL);
	if(ret != 0)
	{
		printf("init thread_mutex err %d\n",ret);
		return ret;
	}

	ret = pthread_cond_init(&thread_crond,NULL);		
	if(0!=ret)
	{
		printf("init thread_crond err %d\n",ret);
		return ret;
	}
	
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);  
	pthread_attr_setstacksize(&thread_attr, 256*1024);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if((ret=pthread_create(&tid, &thread_attr, ThreadTaskMain, this)) != 0 ) {
		printf("pthread_create err %d\n",ret);
		pthread_attr_destroy(&thread_attr); 	
		return ret;
	}

	pthread_attr_destroy(&thread_attr); 		
	strcpy(th_name,name);
	status = STS_IDLE;
	return 0;
}

//pthread_cleanup_push和pthread_cleanup_pop
int ThreadTask::ThreadExit()
{
	if(status==STS_EXIT || status == STS_NO_INIT)
	{
		return 0;		
	}
	
	if(0 == pthread_cancel(tid))
	{
		LOG_TH("pthread_cancel ok\n");
		status = STS_EXIT;
		return 0;
	}
	else
	{
		LOG_TH("pthread_cancel err\n");
		perror("pthread_cancel");
		if(errno == ESRCH)
		{
			status = STS_EXIT;
			return 0;
		}
		else
		{
			perror("ThreadExit");
			return -1;
		}	
	}
}

int ThreadTask::run()
{
	//LOG_TH("run 0\n");
	if(status == STS_EXIT)
		return -1;
	//LOG_TH("run 1\n");
	pthread_mutex_lock(&thread_mutex);	
	while(status!=STS_BUSY)
	{
		//LOG_TH("run 2,status=%d\n",status);
		pthread_cond_wait(&thread_crond,&thread_mutex);
	}
	//LOG_TH("run 3\n");
	if(tf!=NULL)
	{
		pthread_cleanup_push(tf,tf_parm);	
		func(parm);		
		pthread_cleanup_pop(1);
	}
	else
	{
		func(parm);	
	}
	
	LOG_TH("TaskEnd from %s\n",th_name);
	last_idle_tick = GetSysBootSeconds();
	status = STS_IDLE;
	pthread_mutex_unlock(&thread_mutex);

	return 0;
}
/*
当线程异常退出,导致f中途退出(线程取消点处 pthread_cancel ),会调用函数tfree,用于收尾处理,如解锁metux,释放内存等处理
*/
int ThreadTask::AddTask(THREAD_TASK f,void *p,taskfree tfree,void *tfree_parm)
{
	if(status!=STS_IDLE)
		return -1;
		
	int ret = pthread_mutex_trylock(&thread_mutex);	
	if(ret == 0)
	{		
		parm = p;
		func = f;
		status = STS_BUSY;
		tf = tfree;
		tf_parm = tfree_parm;
		LOG_TH("AddTask to %s\n",th_name);
		pthread_mutex_unlock(&thread_mutex);
		pthread_cond_signal(&thread_crond);
		return 0;
	}
	return -1;
}







