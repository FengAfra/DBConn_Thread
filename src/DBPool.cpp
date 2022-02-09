#include "DBPool.h"
#include "DBConn.h"


/******************************************************************************************/
/*********************************
 * 函数：CDBPool
 * 功能：构造函数，赋值数据库连接的变量
 * 入参：
 * 		const char * pool_name：连接池的名字
 *		const char * db_server_ip：此连接池所连接的数据库IP
 *		uint16_t db_server_port：此连接池所连接的数据库端口
 *		const char * username：此连接池所连接的数据库用户名
 *		const char * password：此连接池所连接的数据库密码
 *		const char * db_name：此连接池锁连接的数据库实例
 *		int max_conn_cnt：此连接池所允许的最大连接数
 * 返回值：
 * 		无
*********************************/
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
	if (max_conn_cnt <= MIN_DB_CONN_CNT)
		m_db_max_conn_cnt = MIN_DB_CONN_CNT;

	//初始化最小连接数
	m_db_cur_conn_cnt = MIN_DB_CONN_CNT;
}

/*********************************
 * 函数：~CDBPool
 * 功能：析构函数，析构连接池中的CDBConn连接对象实例
 * 入参：
 *		无
 * 返回值：
 *		无
*********************************/
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

/*********************************
 * 函数：Init
 * 功能：初始化函数，根据最小连接数量构造CDBConn对象实例
 * 入参：
 * 		无
 * 返回值：
 * 		成功：0
 *		失败：-1
*********************************/
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

/*********************************
 * 函数：GetDBConn
 * 功能：获取此连接池的空闲连接，一直等待到有空先连接
 * 入参：
 * 		无
 * 返回值：
 * 		CDBConn*：返回数据库连接
*********************************/
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

/*********************************
 * 函数：RelDBConn
 * 功能：归还数据库连接给此连接池
 * 入参：
 * 		CDBConn * pConn：数据库连接
 * 返回值：
 * 		无
*********************************/
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

/*********************************
 * 函数：GetInstance
 * 功能：单例模式，返回CDBManager单一实例
 * 入参：
 * 		无
 * 返回值：
 * 		CDBManager&：单一实例
*********************************/
CDBManager& CDBManager::GetInstance() {
	static CDBManager instance;
	//instance.Init(confPath);
	//if(instance.Init())
	//	instance = NULL;
	return instance;
}

/*********************************
 * 函数：Init
 * 功能：初始化函数，根据配置文件路径来构建连接池，可以构建多个连接池
 * 入参：
 * 		const char* confPath：连接池配置文件
 * 返回值：
 * 		成功：0
 *		失败：-1
*********************************/
int CDBManager::Init(const char* confPath) {

	/*
	1、读取配置文件，获取相关数据库连接池的配置（待定）
	2、新建配置文件中的所有连接池
	*/
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


/*********************************
 * 函数：GetDBConn
 * 功能：根据连接池的名字来获取此连接池的空闲数据库连接
 * 入参：
 * 		const char * dbpool_name：连接池的名字
 * 返回值：
 * 		CDBConn*：数据库连接
*********************************/
CDBConn* CDBManager::GetDBConn(const char * dbpool_name) {
	auto iter = m_dbpool_map.find(dbpool_name);
	if(iter != m_dbpool_map.end()) {
		return iter->second->GetDBConn();
	}
	else {
		return NULL;
	}
}

/*********************************
 * 函数：RelDBConn
 * 功能：归还数据库连接给连接池
 * 入参：
 * 		CDBConn * pConn：数据库连接
 * 返回值：
 * 		无
*********************************/
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


