#include "DBPool.h"
#include "DBConn.h"


/******************************************************************************************/
CDBPool::CDBPool(const char * pool_name, 
						const char * db_server_ip, 
						uint16_t db_server_port, 
						const char * username, 
						const char * password, 
						const char * db_name, 
						int max_conn_cnt) {

	m_pool_name = pool_name;
	m_db_server_ip = db_server_ip;
	m_db_server_port = db_server_port;
	m_username = username;
	m_password = password;
	m_db_name = db_name;
	m_db_max_conn_cnt = max_conn_cnt;

	//初始化最小连接数
	m_db_cur_conn_cnt = MIN_DB_CONN_CNT;
}

CDBPool::~CDBPool() {

	/*
	1、将m_free_list中的连接释放
	*/
	for(auto iter = m_free_list.begin(); iter != m_free_list.end(); iter++ ){
		CDBConn* pDBConn = (*iter);
		delete pDBConn;
	}
	m_free_list.clear();
}

int CDBPool::Init() {
	/*
	1、创建最小的连接数，每一个连接都是一个CDBConn实例
	2、每一个连接实例都需要绑定连接池的变量，需要在连接内部获取连接池内部信息，并对具体某一个连接实例做处理
	3、调用连接类的初始化函数
	4、将新增数据库连接添加到空闲列表中
	*/
	for(int i = 0; i < m_db_cur_conn_cnt; i++) {
		CDBConn *pDBConn = new CDBConn(this);

		int ret = pDBConn->Init();
		if(ret) {
			delete pDBConn;
			pDBConn = NULL;
			return ret;
		}
		m_free_list.push_back(pDBConn);
	}
	sLogMessage("db pool: %s, size: %d", LOGLEVEL_DEBUG, m_pool_name.c_str(), (int)m_free_list.size());
	return 0;
}

CDBConn* CDBPool::GetDBConn() {
	/*
	1、对工作队列进行加锁，工作队列是共有资源，需要加mutex锁
	2、查看m_free_list是否为空，如果为空：
		1、如果当前连接数小于最大连接数，则申请新连接加入到m_free_list中，
		2、如果大于等于最大连接数，则等待连接释放，使用cond来进行wait
	3、从工作队列中拿去一个工作连接来使用并返回
	4、解锁
	*/
	m_free_lock.Lock();

	while(m_free_list.empty()) {
		if(m_db_cur_conn_cnt >= m_db_max_conn_cnt) {
			m_free_lock.wait();
		}
		else {
			CDBConn* pDBConn = new CDBConn(this);
			int ret = pDBConn->Init();
			if(ret) {
				sLogMessage("pDBConn init failed", LOGLEVEL_ERROR);
				delete pDBConn;
				pDBConn = NULL;
			}
			else {
				m_free_list.push_back(pDBConn);
				m_db_cur_conn_cnt++;
				sLogMessage("new db connection: %s, conn_cnt:%d", LOGLEVEL_INFO, m_pool_name.c_str(), m_db_cur_conn_cnt);
			}
		}
	}

	CDBConn* pDBConn = m_free_list.front();
	m_free_list.pop_front();
	sLogMessage("GetDBConn db connection: %s, conn_cnt:%d", LOGLEVEL_INFO, m_pool_name.c_str(), pDBConn->GetMysql());
	m_free_lock.UnLock();
	return pDBConn;

}

void CDBPool::RelDBConn(CDBConn * pConn) {

	/*
	1、判断传参是否有误
	2、加锁，mutex
	3、判断m_free_list是否存在此连接，避免重复归还
	4、归还连接，并通知cond
	5、关闭mutex
	*/

	if(pConn == NULL) {
		sLogMessage("pConn is NULL", LOGLEVEL_ERROR);
		return;
	}
	m_free_lock.Lock();

	auto iter = m_free_list.begin();
	for(; iter != m_free_list.end(); iter ++) {
		if((*iter) == pConn)
			break;
	}
	if(iter == m_free_list.end()) {
		m_free_list.push_back(pConn);
	}
	m_free_lock.Signal();
	m_free_lock.UnLock();
	
}

/*****************************************************************************************/
CDBManager::CDBManager() {

}

CDBManager::~CDBManager() {

}

CDBManager& CDBManager::GetInstance() {
	static CDBManager instance;
	instance.Init();
	//if(instance.Init())
	//	instance = NULL;
	return instance;
}

int CDBManager::Init() {

	char pool_name[64];
	char host[64];
	char port[64];
	char dbname[64];
	char username[64];
	char password[64];
    int maxconncnt;
	
	strcpy(pool_name, "MYSQLDB");
	strcpy(host, "192.168.217.136");
	strcpy(port, "3306");
	strcpy(dbname, "workspace");
	strcpy(username, "root");
	strcpy(password, "LSFlian@123");
	maxconncnt = 4;

	if (!host || !port || !dbname || !username || !password ) {
		sLogMessage("not configure db instance: %s", LOGLEVEL_ERROR, pool_name);
		return 2;
	}

	int db_port = atoi(port);
	
	CDBPool* pDBPool = new CDBPool(pool_name, host, db_port, username, password, dbname, maxconncnt);
	if(pDBPool->Init()) {
		sLogMessage("init db instance failed: %s", LOGLEVEL_ERROR, pool_name);
		return 2;
	}
	m_dbpool_map.insert(make_pair(pool_name, pDBPool));

	return 0;
}


CDBConn* CDBManager::GetDBConn(const char * dbpool_name) {
	auto iter = m_dbpool_map.find(dbpool_name);
	if(iter != m_dbpool_map.end()) {
		return iter->second->GetDBConn();
	}
	else {
		return NULL;
	}
}


void CDBManager::RelDBConn(CDBConn * pDBConn) {
	if(pDBConn == NULL) {
		sLogMessage("pDBConn is NULL", LOGLEVEL_ERROR);
		return;
	}

	auto iter = m_dbpool_map.find(pDBConn->GetPoolName());
	if(iter != m_dbpool_map.end()) {
		iter->second->RelDBConn(pDBConn);
	}
}


