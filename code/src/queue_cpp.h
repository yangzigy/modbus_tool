//文件名：queue_cpp.h
//时间：2019-6-12
//版本：	V1.0			2019-06-12 12:14:52
#ifndef QUEUE_CPP_H
#define QUEUE_CPP_H

#include "main.h"

//模板队列
template <class T>
class CQueue
{
public:
	T *q_data; //数据指针
	t_maxs buflen; //缓冲区长度(元素个数)
	t_maxs r; //队首(r)指向还没读的那个字节
	t_maxs w;//对尾(w)指向即将写的那个字节
	t_maxs dlen;//数据的长度
	CQueue()
	{
		q_data=0;
		buflen=0;
		r=0;
		w=0;
		dlen=0;
	}
	t_maxs Queue_set(T &p)	//队成功返回0，失败返回1
	{
		if(dlen<buflen) //若是空的可以写
		{
			q_data[w]=p;
			w++;
			if(w>=buflen) w=0;
			dlen++;
			return 0;
		}
		return 1;
	}
	t_maxs Queue_get(T &p) //出队成功返回0，失败返回非零
	{
		if(dlen>0) //若有数据可以读
		{
			p=q_data[r];
			r++;
			if(r>=buflen) r=0;
			dlen--;
			return 0;
		}
		return 1;
	}
};
#endif

