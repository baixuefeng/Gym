#pragma once
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/* 复制函数实现要点:
1. const左值拷贝构造: 成员复制
2. 右值拷贝构造: 成员"窃取", 赋给自己以后, 把源清除;
3. const左值赋值: 清理自身且复制. 
   通常做法:用const左值拷贝生成一个临时变量, 而后与自身交换.如果简单按先清理再复制, 可能清理后复制时出异常, 
   结果处于半截状态,先拷贝构造一个临时变量, 如果出异常原对象保持不变,而一般交换操作是没有异常的; 另外,这样
   可以复用代码,复制的操作复用const左值拷贝,清理的操作复用析构函数;
4. 右值赋值: 成员交换.
*/

class LRValue
{
public:
	explicit LRValue(int n = 0);
	~LRValue();
	LRValue(const LRValue&);
	LRValue(LRValue&&);
	LRValue& operator =(const LRValue&);
	LRValue& operator =(LRValue&&);

	int m_n;
};

SHARELIB_END_NAMESPACE
