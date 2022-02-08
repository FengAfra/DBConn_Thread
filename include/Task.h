#ifndef _TASK_H_

#define _TASK_H_

#include "ostype.h"

/**
1、纯虚类，必须实现Run函数，工作类，主要用于继承，将各种工作继承此类，便于统一化管理
*/
class CTask {
public:
	CTask() {}
	virtual ~CTask() {}
	virtual void Run() = 0;
private:

};


#endif

