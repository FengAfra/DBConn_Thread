
#include "ThreadPool.h"


/***********************************************************************************/

CThreadPool::CThreadPool() {
	m_worker_list = NULL;
	m_worker_size = 0;
}

CThreadPool::~CThreadPool() {

}

int CThreadPool::Init(uint32_t worker_num) {
	/*
	1、初始化线程池
	2、初始化锁，对任务队列进行加锁(这部分合并到初始化工作队列中,初始化各个工作对象的时候，就已经初始化了锁)
	3、初始化工作队列，也就是线程
	*/
	m_worker_size = worker_num;
	m_worker_list = new CWorkerThread[m_worker_size];
	if(!m_worker_list) {
		sLogMessage("m_worker_list init failed", LOGLEVEL_INFO);
		return -1;
	}

	for(uint32_t i =0; i < m_worker_size; i ++) {
		m_worker_list[i].SetThreadIdx(i);
		m_worker_list[i].Start();
	}
	
	return 0;
}


void CThreadPool::AddTask(CTask * pTask) {
	uint32_t thread_idx = random() % m_worker_size;	
	m_worker_list[thread_idx].PushTask(pTask);
}

void CThreadPool::Destory() {
	/*
	1、删除list中所有的工作者，也就是清空工作队列
	*/
	
	if(m_worker_list) {
		delete[] m_worker_list;
	}
}



