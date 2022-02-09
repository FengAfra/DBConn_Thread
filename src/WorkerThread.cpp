#include "WorkerThread.h"


CSLog g_demolog = CSLog(false);


/************************************************************************************/
/*********************************
 * 函数：CWorkerThread
 * 功能：类初始化
 * 入参：
 * 返回值：
*********************************/
CWorkerThread::CWorkerThread() {
	m_task_size = 0;
}

/*********************************
 * 函数：~CWorkerThread
 * 功能：类析构函数
 * 入参：
 * 返回值：
*********************************/
CWorkerThread::~CWorkerThread() {

}

/*********************************
 * 函数：Start
 * 功能：线程池中的创建线程的函数
 * 入参：
 * 			无
 * 返回值：
 * 			无
*********************************/
void CWorkerThread::Start() {

	pthread_create(&m_thread_id, NULL, StartRoutine, this);
}

/*********************************
 * 函数：StartRoutine
 * 功能：静态函数，用作线程回调，线程池中所有线程的回调函数，通过此回调函数调用Execute，也可以直接调用Execute，不过Execute函数中的所有非静态需要修改为静态。
 * 入参：
 * 		void* arg：CWorkerThread对象实体，通过此对象来确定是哪个在执行
 * 返回值：
 * 		void*:
*********************************/
void* CWorkerThread::StartRoutine(void * arg) {

	CWorkerThread * pWorkerThread = (CWorkerThread*) arg;
	pWorkerThread->Execute();
	return NULL;
}

/*********************************
 * 函数：Execute
 * 功能：线程真正执行的函数，通过此函数来获取任务队列中的任务，并进行任务的回调函数，执行功能
 * 入参：
 * 		无
 * 返回值：
 * 		无
*********************************/
void CWorkerThread::Execute() {
	/*
	1、工作线程一直在等待任务到来，使用while(true)
	2、加互斥锁，因为任务队列是公共资源，避免多个任务（线程）处理一个任务
	3、添加条件等待，多个线程等待信号触发
	4、获取当前任务队列的头节点作为需要处理的任务
	5、解互斥锁
	6、调用任务的回调函数
	*/
	while(true) {

		m_thread_lock.Lock();
		while(m_task_list.empty()) {
			m_thread_lock.wait();
		}

		CTask* pTask = m_task_list.front();
		if(!pTask) {
			sLogMessage("pTask failed", LOGLEVEL_INFO);
			delete pTask;
			continue;
		}
		m_task_list.pop_front();

		m_thread_lock.UnLock();

		pTask->Run();
		m_task_size ++;
		sLogMessage("%d have the execute %d task", LOGLEVEL_INFO, m_thread_idx, m_task_size);
	}

}

/*********************************
 * 函数：PushTask
 * 功能：向任务队列中添加任务
 * 入参：
 * 		CTask * pTask：任务实例
 * 返回值：
 * 		无
*********************************/
void CWorkerThread::PushTask(CTask * pTask) {

	/*
	1、加互斥锁，因为任务队列是公共资源
	2、向任务队列中增加任务
	3、发出信号，用于对随机一个响应的锁进行解锁
	4、解互斥锁
	*/
	if(pTask == NULL) {
		sLogMessage("pTask is NULL", LOGLEVEL_INFO);
		return;
	}

	m_thread_lock.Lock();
	m_task_list.push_back(pTask);
	m_thread_lock.Signal();
	m_thread_lock.UnLock();
}




