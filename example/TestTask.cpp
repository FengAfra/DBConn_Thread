#include "TestTask.h"

CTestTask::CTestTask(sql_hander_t sql_function, void * hander_date) {
	m_hander_date = hander_date;
	m_sql_hander = sql_function;
}


CTestTask::~CTestTask() {


}

void CTestTask::Run() {
	if(m_sql_hander) {
		m_sql_hander(m_hander_date);
	}
}



