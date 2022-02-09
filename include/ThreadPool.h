#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_


#include "Task.h"
#include "ThreadLock.h"
#include "ostype.h"
#include "WorkerThread.h"

class CThreadPool {
public:

	CThreadPool();
	~CThreadPool();
	int Init(uint32_t worker_size);
	void AddTask(CTask* pTask);
	void Destory();
	
private:
	//任务队列
	//任务数量
	CWorkerThread*	m_worker_list;
	uint32_t		m_worker_size;
};

#endif

