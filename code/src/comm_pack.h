/*
文件名：comm_pack.h
作者：3817
时间：2014-8-29
版本：	V1.1 2019-01-15

功能：

*/
#ifndef COMM_PACK_H
#define COMM_PACK_H

#include "main.h"

class CComm_Pack
{
public:
	CComm_Pack();
	~CComm_Pack();
	u8 *rec_buff;
	s64 buf_len;//缓冲区长度
	u8 *SYNC;//同步字
	s64 syncbuf_len;//同步字长度
	s64 pack_len;//包全长
	s64 pre_offset;//确定整包长度的位置
	s64 pre_p;
	virtual s64 pre_pack_len(u8 *b,s64 len){return 0;}//返回整包长度
	virtual s64 pro_pack(u8 * b,s64 len){return 0;}//返回是否正确接收
	virtual void lostlock_cb(u8 b){}//失锁回调

	//组包函数
	void pack(u8 *p,s64 l);
};
class CLine_Pack //按包尾组包，可用于提取行
{
public:
	CLine_Pack()
	{
		rec_buff=0;
		buf_len=0;
		pre_p=0;
		endc='\n';
	}
	~CLine_Pack(){}
	u8 *rec_buff;
	u32 buf_len;//缓冲区长度
	u8 endc;//同步字
	u32 pre_p;
	virtual u32 pro_pack(u8 *p,u32 n){return 0;}//返回是否正确接收
	//组包函数
	void pack(u8 *p,u32 n);
};

#endif

