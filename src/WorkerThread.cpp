#include "WorkerThread.h"


CSLog g_demolog = CSLog(false);


/************************************************************************************/
CWorkerThread::CWorkerThread() {
	m_task_size = 0;
}

CWorkerThread::~CWorkerThread() {

}

void CWorkerThread::Start() {

	pthread_create(&m_thread_id, NULL, StartRoutine, this);
}


void* CWorkerThread::StartRoutine(void * arg) {

	CWorkerThread * pWorkerThread = (CWorkerThread*) arg;
	pWorkerThread->Execute();
	return NULL;
}

void CWorkerThread::Execute() {
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

void CWorkerThread::PushTask(CTask * pTask) {

	if(pTask == NULL) {
		sLogMessage("pTask is NULL", LOGLEVEL_INFO);
		return;
	}

	m_thread_lock.Lock();
	m_task_list.push_back(pTask);
	m_thread_lock.Signal();
	m_thread_lock.UnLock();
}




