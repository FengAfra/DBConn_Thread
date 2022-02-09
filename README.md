	ostype.h：主要定义头文件，回调函数类型，以及相关日志定义

	Task.h：定义了CTask类，主要用于业务继承，继承此类，实现纯虚函数run方法，实现多线程的调用。
		对外暴漏接口：
			virtual void Run() = 0：纯虚函数，子类必须实现。作为调用
	
	ThreadLock.h：定义了CThreadLock类，主要负责对多线程的共有资源加锁服务，含有mutex锁和条件变量cond
		1、对外暴漏接口：
			void Lock()：mutex加锁
			void UnLock()：mutex解锁
			void wait()：cond条件变量等待唤醒
			void Signal()：cond条件变量唤醒
	
	WorkerThread.h：定义了CWorkerThread类，主要负责对任务队列的封装使用以及工作线程的使用方法。
		1、对外暴漏接口：
			1、对线程池的函数处理：	
				void Start()：线程池中的创建线程的函数
				void* StartRoutine(void * arg)：静态函数，用作线程回调，线程池中所有线程的回调函数，通过此回调函数调用Execute，也可以直接调用Execute，不过Execute函数中的所有非静态需要修改为静态。
				void Execute()：线程真正执行的函数，通过此函数来获取任务队列中的任务，并进行任务的回调函数，执行功能
			2、对任务队列的函数处理：
				void PushTask(CTask * pTask)：向任务队列中添加任务

	ThreadPool.h：定义了CThreadPool类，主要负责线程池的封装。
		1、对外暴漏接口：
			int Init(uint32_t worker_size)：初始化函数，对线程池的初始化，以及线程池中线程的构造
			void AddTask(CTask* pTask)：根据任务来确定给哪个线程执行，然后将任务CTask对象放置在线程对象CWorkerThread的任务队列中
			void Destory()：删除线程池，对线程池中的各个线程实例进行删除

	DBConn.h：定义了mysql一系列类，来实现单一的mysql连接
		1、CResultSet类：主要实现对mysql查询结果集的存放。便于前台业务获取结果集
		2、CPrepareStatement类：主要实现mysql预处理的内容，比如绑定预处理参数以及预处理语句的执行
		3、CDBConn类：主要实现正常的mysql函数调用。
			int Init()：初始化函数，主要是mysql的连接以及属性设置
			const char* GetPoolName()：获取连接池名字，可以有多个连接池
			CResultSet* ExecuteQuery(const char* sql_query)：执行查询sql语句
			bool ExecuteUpdate(const char* sql_query)：执行更新sql语句：包括delete，update，insert
			bool ExecuteCreate(const char* sql_query)：数据库连接实例的创建表函数
			bool ExecuteDrop(const char* sql_query)：数据库连接实例的删除表函数
			bool StartTransaction()：开启事务
			bool Rollback()：事务结束回退
			bool Commit()：事务结束提交

	DBPool.h：实现了两个类：一个是CDBCPool连接池类，对连接池进行管理。一个是CDBManager类，单例设计模式，并在此类中管理多个连接池
		1、CDBCPool类：连接池自身管理，连接池的申请连接和归还连接
			CDBPool(const char* pool_name, const char* db_server_ip, uint16_t db_server_port, const char* username, const char* password, const char* db_name, int max_conn_cnt)：构造函数，赋值数据库连接的变量
			virtual ~CDBPool()：析构函数，析构连接池中的CDBConn连接对象实例
			int Init()：初始化函数，根据最小连接数量构造CDBConn对象实例
			CDBConn* GetDBConn()：获取此连接池的空闲连接，一直等待到有空先连接
			void RelDBConn(CDBConn* pConn)：归还数据库连接给此连接池
		2、CDBManager类：连接池总体管理，判断使用何种连接池
			static CDBManager& GetInstance()：单例模式，返回CDBManager单一实例
			int Init(const char* confPath)：初始化函数，根据配置文件路径来构建连接池，可以构建多个连接池
			CDBConn* GetDBConn(const char* dbpool_name)：根据连接池的名字来获取此连接池的空闲数据库连接
			void RelDBConn(CDBConn* pDBConn)：归还数据库连接给连接池



first version success
second version: config read，threadpool的线程扩缩容，threadpool的判断是否完成当前任务
更新时间待定