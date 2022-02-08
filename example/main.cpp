#include "ThreadPool.h"
#include "DBPool.h"
#include "TestTask.h"

#define THREAD_NUM	16

static CThreadPool g_thread_pool;


int main() {

	g_thread_pool.Init(THREAD_NUM);

	CDBManager* pDBManager = &(CDBManager::GetInstance());
	if (!pDBManager) {
		sLogMessage("DBManager init failed", LOGLEVEL_ERROR);
		return -1;
	}
	sLogMessage("db init success", LOGLEVEL_INFO);

	return 0;

}


