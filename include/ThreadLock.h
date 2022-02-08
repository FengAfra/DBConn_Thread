#ifndef _THREAD_LOCK_H_
#define _THREAD_LOCK_H_

#include "ostype.h"
#include "ThreadLock.h"

/**
1、线程池所需要使用的锁为互斥锁，以及条件变量
2、互斥锁对共有内容进行加锁
3、条件变量用于有任务到来时对工作线程的唤醒。且cond条件变量已经自己解决惊群现象
*/
class CThreadLock {
public:
	CThreadLock() {
		pthread_mutex_init(&m_mutex, NULL);
		pthread_cond_init(&m_cond, NULL);
	}
	~CThreadLock() {
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);		
	}

	void Lock() {pthread_mutex_lock(&m_mutex);}
	void UnLock() {pthread_mutex_unlock(&m_mutex);}
	void wait() {pthread_cond_wait(&m_cond, &m_mutex);}
	void Signal() {pthread_cond_signal(&m_cond);}

private:

	pthread_mutex_t		m_mutex;
	pthread_cond_t		m_cond;

};


#endif

