/*
文件名：comm_pack.h
作者：3817
时间：2014-8-29
版本：	V1.1 2019-01-15
		V1.2 2020-08-19
*/
#ifndef COMM_PACK_H
#define COMM_PACK_H

#include "main.h"

//注意：
//	确定包长时，最小整包长度需大于等于pre_offset+2
class CComm_Pack
{
public:
	CComm_Pack();
	u8 *rec_buff;
	u32 buf_len;//缓冲区长度
	u8 *SYNC;//同步字
	u32 syncbuf_len;//同步字长度
	u32 pack_len;//包全长
	u32 pre_offset;//确定整包长度的偏移位置
	u32 pre_p;
	virtual u32 pre_pack_len(u8 *p,u32 len){return 0;}//返回整包长度
	virtual u32 pro_pack(u8 *p,u32 len){return 0;}//返回是否正确接收
	virtual void lostlock_cb(u8 b){}//失锁回调

	//组包函数
	void pack(u8 *p,u32 l);
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

