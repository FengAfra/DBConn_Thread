#include "ThreadPool.h"
#include "DBPool.h"
#include "TestTask.h"
#include <unistd.h>


#define THREAD_NUM	10

static CThreadPool g_thread_pool;
CDBManager* pDBManager;


static string int2string(uint32_t user_id)
{
    stringstream ss;
    ss << user_id;
    return ss.str();
}


#define DROP_IMUSER_TABLE "DROP TABLE IF EXISTS IMUser" /* if EXISTS 好处 是如果表不存在,执行不会报错 */
                                                        

#define CREATE_IMUSER_TABLE "CREATE TABLE IMUser (     \
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT '用户id',   \
  `sex` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '1男2女0未知', \
  `name` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '用户名',  \
  `domain` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '拼音',  \
  `nick` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '花名,绰号等', \
  `password` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '密码',    \
  `salt` varchar(4) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '混淆码',   \
  `phone` varchar(11) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '手机号码',   \
  `email` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'email',  \
  `company` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '公司名称', \
  `address` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '所在地区', \
  `avatar` varchar(255) COLLATE utf8mb4_bin DEFAULT '' COMMENT '自定义用户头像',    \
  `validateMethod` tinyint(2) unsigned DEFAULT '1' COMMENT '好友验证方式',  \
  `departId` int(11) unsigned NOT NULL DEFAULT '1' COMMENT '所属部门Id',    \
  `status` tinyint(2) unsigned DEFAULT '0' COMMENT '1. 试用期 2. 正式 3. 离职 4.实习',  \
  `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',   \
  `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',   \
  `push_shield_status` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '0关闭勿扰 1开启勿扰',  \
  `sign_info` varchar(128) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '个性签名',  \
  PRIMARY KEY (`id`),   \
  KEY `idx_domain` (`domain`),  \
  KEY `idx_name` (`name`),  \
  KEY `idx_phone` (`phone`) \
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;"


//typedef unsigned int atomic_uint;

//static atomic_uint IMUser_nId = 0;

static unsigned int IMUser_nId = 0;


// 初始化原子变量
#define atomic_init(obj, value) \
    do                          \
    {                           \
        *(obj) = (value);       \
    } while (0)

// 原子变量加操作
#define atomic_fetch_add(object, operand) \
    __sync_fetch_and_add(object, operand)
// 原子变量减操作
#define atomic_fetch_sub(object, operand) \
    __sync_fetch_and_sub(object, operand)

bool insertUser(CDBConn* pDBConn) {
	bool bRet = false;
	string strSql;
	strSql = "insert into IMUser(`salt`,`sex`,`nick`,`password`,`domain`,`name`,`phone`,`email`,`company`,`address`,`avatar`,`sign_info`,`departId`,`status`,`created`,`updated`) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	atomic_fetch_add(&IMUser_nId, 1);
	int id_index = IMUser_nId;
	
	CPrepareStatement *stmt = new CPrepareStatement();
	if (stmt->Init(pDBConn->GetMysql(), strSql))
	{
		uint32_t nNow = (uint32_t)time(NULL);
		uint32_t index = 0;
		string strOutPass = "987654321";
		string strSalt = "abcd";

        int nSex = 1;                             // 用户性别 1.男;2.女
        int nStatus = 0;                          // 用户状态0 正常， 1 离职
        uint32_t nDeptId = 0;                     // 所属部门
        string strNick = "Dancy";                  // 花名
        string strDomain = "Dancy";             // 花名拼音
        string strName = "liansf";                // 真名
        string strTel = "1857036";            // 手机号码
        string strEmail = "326873713@qq.com";     // Email
        string strAvatar = "";                    // 头像
        string sign_info = "You have to be strong enough to respect your principles and bottom line";          //个性签名
        string strPass = "123456";                //密码
        string strCompany = "Dancy";           //公司
        string strAddress = "北京市"; //地址

		stmt->SetParam(index++, strSalt);
		stmt->SetParam(index++, nSex);
		stmt->SetParam(index++, strNick);
		stmt->SetParam(index++, strOutPass);
		stmt->SetParam(index++, (strDomain + int2string(id_index)));
		stmt->SetParam(index++, (strName + int2string(id_index)));
		stmt->SetParam(index++, (strTel+ int2string(id_index)));
		stmt->SetParam(index++, strEmail);
		stmt->SetParam(index++, strCompany);
		stmt->SetParam(index++, strAddress);
		stmt->SetParam(index++, strAvatar);
		stmt->SetParam(index++, sign_info);
		stmt->SetParam(index++, nDeptId);
		stmt->SetParam(index++, nStatus);
		stmt->SetParam(index++, nNow);
		stmt->SetParam(index++, nNow);
		bRet = stmt->ExecuteUpdate();

		if (!bRet)
		{
			sLogMessage("insert user failed: %s\n", LOGLEVEL_ERROR, strSql.c_str());
		}
		else
		{
			uint32_t nId = (uint32_t)stmt->GetInsertId();
			sLogMessage("register then get user_id:%d\n", LOGLEVEL_INFO, nId);
		}
	}
	delete stmt;

	return true;
}

void insert_functionUser(void *arg)
{
    char* pool_name = (char*)arg;
	CDBConn* pDBConn = pDBManager->GetDBConn(pool_name);
	if(pDBConn) {
		if(!insertUser(pDBConn)) {
			sLogMessage("insertUser failed", LOGLEVEL_ERROR);
		}
		else {
			sLogMessage("insertUser success", LOGLEVEL_INFO);
		}
	}
	else {
		sLogMessage("GetDBConn failed", LOGLEVEL_ERROR);
	}
	pDBManager->RelDBConn(pDBConn);
}


int main() {

	g_thread_pool.Init(THREAD_NUM);

	pDBManager = &(CDBManager::GetInstance());
	if (!pDBManager) {
		sLogMessage("DBManager init failed", LOGLEVEL_ERROR);
		return -1;
	}
	sLogMessage("db init success", LOGLEVEL_INFO);

	

	char pool_name[64];
	memset(pool_name, 0, sizeof(pool_name));
	strcpy(pool_name, "MYSQLDB");

	CDBConn *pDBConn = pDBManager->GetDBConn(pool_name);
	if (pDBConn)
    {
        bool ret = pDBConn->ExecuteDrop(DROP_IMUSER_TABLE); // 删除表
        if (ret)
        {
            sLogMessage("DROP_IMUSER_TABLE ok\n", LOGLEVEL_INFO);
        }
        // 1. 创建表
        ret = pDBConn->ExecuteCreate(CREATE_IMUSER_TABLE);  // 创建表
        if (ret)
        {
            sLogMessage("CREATE_IMUSER_TABLE ok\n", LOGLEVEL_INFO);
        }
        else
        {
            sLogMessage("ExecuteCreate failed\n", LOGLEVEL_ERROR);
            return -1;
        }
    }
   	pDBManager->RelDBConn(pDBConn);

	for (int i = 0; i < 100; i++) {
		CTask* pTask = new CTestTask( insert_functionUser, (void*)pool_name);
		g_thread_pool.AddTask(pTask);
	}
	sleep(10);

	g_thread_pool.Destory();
	return 0;
	
}


