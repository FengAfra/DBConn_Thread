#include "DBConn.h"


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





