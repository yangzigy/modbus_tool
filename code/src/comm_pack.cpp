/*
文件名：comm_pack.cpp
作者：3817
时间：2014-8-29
版本：	V1.1 2019-01-15

功能：

*/
#include "comm_pack.h"

CComm_Pack::CComm_Pack()
{
	rec_buff=0;
	buf_len=0;
	SYNC=0;
	syncbuf_len=0;
	pack_len=0;
	pre_offset=0;
	pre_p=0;
}
CComm_Pack::~CComm_Pack()
{
	if(rec_buff)
	{
		delete[] rec_buff;
	}
}

void CComm_Pack::pack(u8 *p,s64 l)
{
	while(l>0)
	{
		if(pre_p<syncbuf_len)//正在寻找包头
		{
			if(*p==SYNC[pre_p])//引导字正确
			{
				rec_buff[pre_p++]=*p;
			}
			else
			{
				lostlock_cb(*p);//失锁回调
				pre_p=0;
			}
			l--;
			p++;
		}
		else if(pre_p<pre_offset)//确定不同包的长度
		{
			rec_buff[pre_p++]=*p;
			l--;
			p++;
			if (pre_p>=pre_offset)
			{
				pack_len=pre_pack_len(rec_buff, pre_p);
				if (pack_len<pre_p)//防止返回包长小，导致段错误
				{
					pack_len=pre_p;
				}
			}
		}
		else//正常接收数据包
		{
			//可用数据量：l
			//需要的数据量：
			s64 need=pack_len-pre_p;
			if (need>0)
			{
				s64 r=need>l?l:need;//取其中比较小的
				memcpy(rec_buff+pre_p,p,r);
				pre_p+=r;
				p+=r;
				l-=r;
			}
			if(pre_p>=pack_len)
			{
				//调用处理函数
				if(pro_pack(rec_buff, pack_len))
				{//若接收不正确
					pre_p=0;
					lostlock_cb(rec_buff[0]);//失锁回调
					pack(rec_buff+1,pack_len-1);
				}
				else
				{
					pre_p=0;
				}
			}
		}
	}
}
//////////////////////////////////////////////////////
//按包尾组包，可用于提取行
void CLine_Pack::pack(u8 *p,u32 n)
{
	int i;
	for(i=0;i<n;i++)
	{
		if(pre_p>=buf_len-1)
		{
			pre_p=buf_len-2;
		}
		u8 c=p[i];
		if(c==endc)	//
		{
			rec_buff[pre_p++]='\0';
			pro_pack(rec_buff,pre_p); //处理
			//处理完成，需要将指针归零
			pre_p=0;
		}
		else		//若是普通字符
		{
			rec_buff[pre_p++]=c;
		}
	}
}
