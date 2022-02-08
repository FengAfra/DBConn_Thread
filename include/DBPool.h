#ifndef _DB_POOL_H_
#define _DB_POOL_H_

#include "ThreadPool.h"
#include <mysql.h>

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



class CDBPool {
public:
	CDBPool(const char* pool_name, const char* db_server_ip, uint16_t db_server_port, const char* username, 
				const char* password, const char* db_name, int max_conn_cnt);
	virtual ~CDBPool();

	int Init();

	CDBConn* GetDBConn();
	void RelDBConn(CDBConn* pConn);


	const char* GetPoolName() { return m_pool_name.c_str(); }
	const char* GetDBServerIP() { return m_db_server_ip.c_str(); }
	uint16_t GetDBServerPort() { return m_db_server_port; }
	const char* GetUsername() { return m_username.c_str(); }
	const char* GetPasswrod() { return m_password.c_str(); }
	const char* GetDBName() { return m_db_name.c_str(); }

private:

	string 		m_pool_name;
	string		m_db_server_ip;
	uint16_t	m_db_server_port;
	string		m_username;
	string		m_password;
	string		m_db_name;

	int			m_db_max_conn_cnt;
	int			m_db_cur_conn_cnt;

	list<CDBConn*> m_free_list;

	CThreadLock	m_free_lock;
	
};



class CDBManager {
public:
	virtual ~CDBManager();
	
	static CDBManager& GetInstance();

	int Init();

	CDBConn* GetDBConn(const char* dbpool_name);
	void RelDBConn(CDBConn* pDBConn);

private:
	CDBManager();
	CDBManager(const CDBManager&)=default;
	CDBManager& operator=(const CDBManager&)=default;

private:
	map<string, CDBPool*>	m_dbpool_map;
	
};

#endif


