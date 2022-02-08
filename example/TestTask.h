#ifndef _TEST_TASK_H_
#define	_TEST_TASK_H_

#include "DBPool.h"

class CTestTask :public CTask {

public:
	CTestTask(sql_hander_t sql_function, void* hander_date);
	virtual ~CTestTask();

	virtual void Run();

private:

	sql_hander_t	m_sql_hander;
	void*			m_hander_date;

};


#endif

