#include "DBPool.h"

CResultSet::CResultSet(MYSQL_RES* res)
{
	m_res = res;

	// map table field key to index in the result array
	int num_fields = mysql_num_fields(m_res);
	MYSQL_FIELD* fields = mysql_fetch_fields(m_res);
	for(int i = 0; i < num_fields; i++)
	{
	   m_key_map.insert(make_pair(fields[i].name, i));
	}
}

CResultSet::~CResultSet()
{
	if (m_res) {
		mysql_free_result(m_res);
		m_res = NULL;
	}
}

bool CResultSet::Next()
{
	m_row = mysql_fetch_row(m_res);
	if (m_row) {
		return true;
	} else {
		return false;
	}
}

int CResultSet::_GetIndex(const char* key)
{
	map<string, int>::iterator it = m_key_map.find(key);
	if (it == m_key_map.end()) {
		return -1;
	} else {
		return it->second;
	}
}

int CResultSet::GetInt(const char* key)
{
	int idx = _GetIndex(key);
	if (idx == -1) {
		return 0;
	} else {
		return atoi(m_row[idx]);
	}
}

char* CResultSet::GetString(const char* key)
{
	int idx = _GetIndex(key);
	if (idx == -1) {
		return NULL;
	} else {
		return m_row[idx];
	}
}

/////////////////////////////////////////
CPrepareStatement::CPrepareStatement()
{
	m_stmt = NULL;
	m_param_bind = NULL;
	m_param_cnt = 0;
}

CPrepareStatement::~CPrepareStatement()
{
	if (m_stmt) {
		mysql_stmt_close(m_stmt);
		m_stmt = NULL;
	}

	if (m_param_bind) {
		delete [] m_param_bind;
		m_param_bind = NULL;
	}
}

bool CPrepareStatement::Init(MYSQL* mysql, string& sql)
{
	mysql_ping(mysql);

	m_stmt = mysql_stmt_init(mysql);
	if (!m_stmt) {
		sLogMessage("mysql_stmt_init failed", LOGLEVEL_ERROR);
		return false;
	}

	if (mysql_stmt_prepare(m_stmt, sql.c_str(), sql.size())) {
		sLogMessage("mysql_stmt_prepare failed: %s", LOGLEVEL_ERROR, mysql_stmt_error(m_stmt));
		return false;
	}

	m_param_cnt = mysql_stmt_param_count(m_stmt);
	if (m_param_cnt > 0) {
		m_param_bind = new MYSQL_BIND [m_param_cnt];
		if (!m_param_bind) {
			sLogMessage("new failed", LOGLEVEL_ERROR);
			return false;
		}

		memset(m_param_bind, 0, sizeof(MYSQL_BIND) * m_param_cnt);
	}

	return true;
}

void CPrepareStatement::SetParam(uint32_t index, int& value)
{
	if (index >= m_param_cnt) {
		sLogMessage("index too large: %d", LOGLEVEL_ERROR, index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
}

void CPrepareStatement::SetParam(uint32_t index, uint32_t& value)
{
	if (index >= m_param_cnt) {
		sLogMessage("index too large: %d", LOGLEVEL_ERROR, index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
}

void CPrepareStatement::SetParam(uint32_t index, string& value)
{
	if (index >= m_param_cnt) {
		sLogMessage("index too large: %d", LOGLEVEL_ERROR, index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_STRING;
	m_param_bind[index].buffer = (char*)value.c_str();
	m_param_bind[index].buffer_length = value.size();
}

void CPrepareStatement::SetParam(uint32_t index, const string& value)
{
    if (index >= m_param_cnt) {
        sLogMessage("index too large: %d", LOGLEVEL_ERROR, index);
        return;
    }
    
    m_param_bind[index].buffer_type = MYSQL_TYPE_STRING;
    m_param_bind[index].buffer = (char*)value.c_str();
    m_param_bind[index].buffer_length = value.size();
}

bool CPrepareStatement::ExecuteUpdate()
{
	if (!m_stmt) {
		sLogMessage("no m_stmt", LOGLEVEL_ERROR);
		return false;
	}

	if (mysql_stmt_bind_param(m_stmt, m_param_bind)) {
		sLogMessage("mysql_stmt_bind_param failed: %s", LOGLEVEL_ERROR, mysql_stmt_error(m_stmt));
		return false;
	}

	if (mysql_stmt_execute(m_stmt)) {
		sLogMessage("mysql_stmt_execute failed: %s", LOGLEVEL_ERROR, mysql_stmt_error(m_stmt));
		return false;
	}

	if (mysql_stmt_affected_rows(m_stmt) == 0) {
		sLogMessage("ExecuteUpdate have no effect", LOGLEVEL_ERROR);
		return false;
	}

	return true;
}

uint32_t CPrepareStatement::GetInsertId()
{
	return mysql_stmt_insert_id(m_stmt);
}




/******************************************************************************************/
CDBConn::CDBConn(CDBPool * pDBPool) {
	m_pDBPool = pDBPool;
	m_mysql = NULL;
}

CDBConn::~CDBConn() {

}

int CDBConn::Init() {
	/*
	1、mysql初始化，创建一个mysql的上下文指针
	2、设置mysql能够超时实现重新自动链接，和mysql_ping函数一起使用
	3、设置mysql的字符集编码是utf8mb4，是真正意义上的utf-8。之前的utf8只支持2个字节，utf8mb4是能够支持4个字节的
	4、将mysql上下文连接到数据库
	*/
	m_mysql = mysql_init(NULL);
	if(!m_mysql) {
		sLogMessage("mysql_init failed", LOGLEVEL_ERROR);
		return -1;
	}

	bool reconnect = true;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);
	mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if(!mysql_real_connect(m_mysql, m_pDBPool->GetDBServerIP(), m_pDBPool->GetUsername(), m_pDBPool->GetPasswrod(),
			m_pDBPool->GetDBName(), m_pDBPool->GetDBServerPort(), NULL, 0)) {
		sLogMessage("mysql_real_connect failed:%s", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return -1;
	}
	return 0;
}

const char* CDBConn::GetPoolName() {

	return m_pDBPool->GetPoolName();
}

CResultSet* CDBConn::ExecuteQuery(const char * sql_query) {
	mysql_ping(m_mysql);

	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query))) {
		sLogMessage("mysql_real_query failed: %s, sql: %s", LOGLEVEL_ERROR, mysql_error(m_mysql), sql_query);
		return NULL;
	}

	MYSQL_RES* res = mysql_store_result(m_mysql);
	if (!res) {
		sLogMessage("mysql_store_result failed: %s", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return NULL;
	}

	CResultSet* result_set = new CResultSet(res);
	return result_set;

}

bool CDBConn::ExecuteUpdate(const char * sql_query) {
	mysql_ping(m_mysql);
	
	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query))) {
		sLogMessage("mysql_real_query failed: %s, sql: %s", LOGLEVEL_ERROR, mysql_error(m_mysql), sql_query);
		return false;
	}

	if (mysql_affected_rows(m_mysql) > 0) {
		return true;
	} else {
		return false;
	}

}

/*********************************
 * 函数：ExecuteCreate
 * 功能：数据库连接实例的创建表函数
 * 入参：
 * 			无
 * 返回值：
 * 			成功：true
 *			失败：false
*********************************/
bool CDBConn::ExecuteCreate(const char* sql_query) {
	mysql_ping(m_mysql);
	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query)))
	{
		sLogMessage("mysql_real_query failed: %s, sql: start transaction\n", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return false;
	}
	return true;
}

/*********************************
 * 函数：ExecuteDrop
 * 功能：数据库连接实例的删除表函数
 * 入参：
 * 			无
 * 返回值：
 * 			成功：true
 *			失败：false
*********************************/
bool CDBConn::ExecuteDrop(const char* sql_query) {
	mysql_ping(m_mysql);
	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query)))
	{
		sLogMessage("mysql_real_query failed: %s, sql: start transaction\n", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return false;
	}
	return true;
}

bool CDBConn::StartTransaction()
{
	mysql_ping(m_mysql);

	if (mysql_real_query(m_mysql, "start transaction\n", 17))
	{
		sLogMessage("mysql_real_query failed: %s, sql: start transaction\n", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return false;
	}

	return true;
}

bool CDBConn::Rollback()
{
	mysql_ping(m_mysql);

	if (mysql_real_query(m_mysql, "rollback\n", 8))
	{
		sLogMessage("mysql_real_query failed: %s, sql: rollback\n", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return false;
	}

	return true;
}

bool CDBConn::Commit()
{
	mysql_ping(m_mysql);

	if (mysql_real_query(m_mysql, "commit\n", 6))
	{
		sLogMessage("mysql_real_query failed: %s, sql: commit\n", LOGLEVEL_ERROR, mysql_error(m_mysql));
		return false;
	}

	return true;
}



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
	maxconncnt = 2;

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


