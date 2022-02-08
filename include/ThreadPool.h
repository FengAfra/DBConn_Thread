#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_


#include "Task.h"
#include "ThreadLock.h"
#include "ostype.h"

//工作线程类
class CWorkerThread {
public:

	CWorkerThread();
	~CWorkerThread();

	void Start();
	void Execute();
	void PushTask(CTask* pTask);

	static void* StartRoutine(void * arg);

	void SetThreadIdx(uint32_t       idx) {m_thread_idx = idx;}

private:

	//线程ID
	pthread_t		m_thread_id;
	//任务队列，使用list容器
	list<CTask*>	m_task_list;
	//任务数量
	uint32_t		m_task_size;
	//锁对象
	CThreadLock		m_thread_lock;
	//标识CWorkerThread对象的编号
	uint32_t		m_thread_idx;
};


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

