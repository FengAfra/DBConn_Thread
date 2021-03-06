/**
BaiDu：
1、为什么在CWorkerThread类中要先线程回调一个static函数，然后再用传入的对象调用execute函数，为什么不能直接使用，是因为一个线程有一个任务队列吗？
*/

/*
切记：
1、线程池中每一个线程都有一个任务队列
2、可以有多个连接池，每个连接池通过pool_name来辨别
*/

#ifndef _OSTYPE_H_
#define _OSTYPE_H_


#include <iostream>
#include <CLog4cplus/CSLog.h>
#include <pthread.h>
#include <list>


using namespace std;


typedef void (*sql_hander_t)(void * arg);

extern CSLog g_demolog;


#define __FILENAME__  (strrchr(__FILE__, '/')) ? (strrchr(__FILE__, '/' + 1)):( __FILE__ )

#define sLogMessage(fmt, logLevel, args...) g_demolog.LogMessage(fmt, logLevel, __FILENAME__, __LINE__, __FUNCTION__, ##args)



#endif


