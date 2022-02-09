#ifndef _DBCONN_H_
#define _DBCONN_H_

#include "ThreadPool.h"
#include <mysql.h>
#include "DBPool.h"

#define MIN_DB_CONN_CNT		2


class CResultSet {
public:
	CResultSet(MYSQL_RES* res);
	virtual ~CResultSet();

	bool Next();
	int GetInt(const char* key);
	char* GetString(const char* key);
private:
	int _GetIndex(const char* key);

	MYSQL_RES* 			m_res;
	MYSQL_ROW			m_row;
	map<string, int>	m_key_map;
};



/*
 * TODO:
 * 用MySQL的prepare statement接口来防止SQL注入
 * 暂时只用于插入IMMessage表，因为只有那里有SQL注入的风险，
 * 以后可以把全部接口用这个来实现替换
 */
class CPrepareStatement {
public:
	CPrepareStatement();
	virtual ~CPrepareStatement();

	bool Init(MYSQL* mysql, string& sql);

	void SetParam(uint32_t index, int& value);
	void SetParam(uint32_t index, uint32_t& value);
    void SetParam(uint32_t index, string& value);
    void SetParam(uint32_t index, const string& value);

	bool ExecuteUpdate();
	uint32_t GetInsertId();
private:
	MYSQL_STMT*	m_stmt;
	MYSQL_BIND*	m_param_bind;
	uint32_t	m_param_cnt;
};

class CDBPool;

class CDBConn {

public:
	CDBConn(CDBPool* pDBPool);
	~CDBConn();

	int Init();

	const char* GetPoolName();

	CResultSet* ExecuteQuery(const char* sql_query);
	bool ExecuteUpdate(const char* sql_query);
	bool ExecuteCreate(const char* sql_query);
	bool ExecuteDrop(const char* sql_query);
	bool StartTransaction();
	bool Rollback();
	bool Commit();
	MYSQL* GetMysql() { return m_mysql; }
	
private:
	CDBPool*		m_pDBPool;

	MYSQL*			m_mysql;
};


#endif

