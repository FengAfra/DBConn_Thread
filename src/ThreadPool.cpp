
#include "ThreadPool.h"


/***********************************************************************************/

/*********************************
 * 函数：CThreadPool
 * 功能：构造函数
 * 入参：
 * 		无
 * 返回值：
 * 		无
*********************************/
CThreadPool::CThreadPool() {
	m_worker_list = NULL;
	m_worker_size = 0;
}

/*********************************
 * 函数：~CThreadPool
 * 功能：析构函数
 * 入参：
 * 		无
 * 返回值：
 * 		无
*********************************/
CThreadPool::~CThreadPool() {

}

/*********************************
 * 函数：Init
 * 功能：初始化函数，对线程池的初始化，以及线程池中线程的构造
 * 入参：
 * 		uint32_t worker_num：工作线程的个数，可以业务层来定义
 * 返回值：
 * 		成功：0
 *		失败：-1
*********************************/
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

/*********************************
 * 函数：AddTask
 * 功能：根据任务来确定给哪个线程执行，然后将任务CTask对象放置在线程对象CWorkerThread的任务队列中
 * 入参：
 * 		CTask * pTask：任务CTask实例
 * 返回值：
 * 		无
*********************************/
void CThreadPool::AddTask(CTask * pTask) {
	uint32_t thread_idx = random() % m_worker_size;	
	m_worker_list[thread_idx].PushTask(pTask);
}

/*********************************
 * 函数：Destory
 * 功能：删除线程池，对线程池中的各个线程实例进行删除
 * 入参：
 * 		无
 * 返回值：
 * 		无
*********************************/
void CThreadPool::Destory() {
	/*
	1、删除list中所有的工作者，也就是清空工作队列
	*/
	
	if(m_worker_list) {
		delete[] m_worker_list;
	}
}


