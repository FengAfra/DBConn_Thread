#ifndef _DB_POOL_H_
#define _DB_POOL_H_

#include "DBConn.h"

class CDBConn;

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

	int Init(const char* confPath);

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


