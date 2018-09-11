
#include "modbus.h"

///////////////////////////////////////////////////////////////////////////
//				基类
///////////////////////////////////////////////////////////////////////////
#define MODBUS_RX_DATA ((u16*)(headbuf+3))  //客户判断状态为2时，从这个接口取数
#define TP	(*(MODBUS_RTU_REQ*)tx_buf)
#define PACK	(*(MODBUS_RTU_REQ*)p)
u16 GetModbusCRC16(u8 *cp,int leng)
{
	unsigned int j,i,crc=0xFFFF;
	if (leng<=0)
	{
		return 0;
	}
	for (j = 0; j < leng; j++)
	{
		crc = crc^(unsigned int)(cp[j]);
		for(i = 0; i < 8; i++)
		{
			if ((crc&1)!=0 )
			{
				crc=(crc>>1)^0xA001;
			}
			else
			{
				crc=crc>>1;
			}
		}
	}
	return (u16)crc;
}
void modbus_send_void(u8 *p,int n){};
void CModbus::reg(MODBUS_ADDR_LIST *a) //向模块注册地址
{
	MODBUS_ADDR_LIST *tmp=addr_list;
	while(tmp)
	{
		if(tmp==a) return;
		tmp=tmp->next;
	}
	a->next=addr_list;
	addr_list=a;
}
u16 *CModbus::get_data(u16 a) //从地址获得数据,输入小端地址
{
	MODBUS_ADDR_LIST *tmp=addr_list;
	while(tmp)
	{
		if(a>=tmp->st && a<(tmp->st+tmp->num)) //若地址在这个段
		{
			a=a-tmp->st; //相对索引
			return tmp->buf+a;
		}
		tmp=tmp->next;
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////
//				主机
///////////////////////////////////////////////////////////////////////////
s64 CModbus_Master::pre_pack_len(u8 *b,s64 len)//返回整包长度
{
	if(b[1] & 0x80)
	{
		return 5; //若是错误代码
	}
	else if(b[1]==0x10) //若是主机写配置，回复长度一定为8
	{
		len=8;
	}
	else if(b[1]==0x06) //若是写单寄存器
	{
		len=8; //主从都是8
	}
	else
	{
		len=b[2]+5;
		if(len<6) len=6;
	}
	return len;
}
s64 CModbus_Master::pro_pack(u8 *p,s64 len)  //主机接收处理
{
	u16 crc=0;
	if(len<5) //长度错误
	{
		return 1;
	}
	crc=GetModbusCRC16(p,len-2);
	if(crc!=*(u16*)(p+len-2))
	{
		return 1;
	}
	if(p[1]==4 || p[1]==3) //读输入
	{
		if(p[2]!=CHANGE_END16(TP.num)*2)
		{
			return 0;
		}
		if(cur_send) //将收到的数据放在客户提供的缓存中
		{
			int i;
			for(i=0;i<cur_send->num;i++)
			{
				cur_send->buf[i]=CHANGE_END16(MODBUS_RX_DATA[i]);
			}
			cur_send->stat=2;
		}
	}
	else if(p[1]==0x06) //写单寄存器
	{
		if(cur_send) //将收到的数据放在客户提供的缓存中
		{
			cur_send->stat=2;
		}
	}
	else if(p[1]==0x10) //多个写入
	{
		if(cur_send) //将收到的数据放在客户提供的缓存中
		{
			cur_send->stat=2;
		}
	}
	rx_fun(p,len);
	return 0;
}
int CModbus_Master::host_send(u8 addr,u8 fun,u16 st,u16 num,u16 *d) //
{
	u16 crc=0,i;
	address=addr;
	TP.addr=address;
	TP.fun=fun;
	TP.st=CHANGE_END16(st);
	TP.num=CHANGE_END16(num);
	switch(fun)
	{
	case 0x03: //读保持寄存器，8字节
	case 0x04: //读输入寄存器，8字节
		TP.crc=GetModbusCRC16(tx_buf,sizeof(MODBUS_RTU_REQ)-2);
		send_fun((u8*)tx_buf,sizeof(MODBUS_RTU_REQ));
		break;
	case 0x06: //写单寄存器
		TP.num=CHANGE_END16(d[0]);
		TP.crc=GetModbusCRC16(tx_buf,sizeof(MODBUS_RTU_REQ)-2);
		send_fun((u8*)tx_buf,sizeof(MODBUS_RTU_REQ));
		break;
	case 0x10: //批量写
		if(num>MAX_REG_NUM) return 2;
		tx_buf[6]=num*2;
		for(i=0;i<num;i++)
		{
			((u16*)(tx_buf+7))[i]=CHANGE_END16(d[i]);
		}
		crc=GetModbusCRC16(tx_buf,tx_buf[6]+7);
		*(u16*)(tx_buf+tx_buf[6]+7)=crc;
		send_fun((u8*)tx_buf,tx_buf[6]+9);
		break;
	default:
		return 3;
	}
	return 0;
}
void CModbus_Master::poll(void) //周期函数，主机通过周期函数进行发送
{
	MODBUS_ADDR_LIST *tmp=addr_list;
	MODBUS_ADDR_LIST *psend=0;
	while(tmp) //对于每一个任务
	{
		if(tmp->freq==0) //若是单次任务
		{
			if(tmp->tick>1) //有值就应该发送
			{
				psend=tmp;
			}
		}
		else if(tmp->tick==tmp->freq) //不是单次，且需要发送
		{
			psend=tmp;
		}
		else //不是单次，也不用发送
		{
			tmp->tick++;
		}
		//判断各任务的正确性
		if(tmp->stat==1 || tmp->stat==3) //若上次正在发送，现在没有接收,或错误
		{
			tmp->err=tmp->stat;
		}
		else if(tmp->stat==2) //若正确回复
		{
			tmp->err=0;
		}
		tmp->stat=0; //清空为空闲状态
		tmp=tmp->next;
	}
	if(psend) //若有任务需要发送
	{
		psend->tick=1;
		host_send(psend->addr,psend->type,psend->st,psend->num,psend->buf);
		psend->stat=1;
		//先清空接收变量
		pre_p=0;
	}
	
	cur_send=psend;
}
///////////////////////////////////////////////////////////////////////////
//				从机
///////////////////////////////////////////////////////////////////////////
s64 CModbus_Slave::pre_pack_len(u8 *b,s64 len)//返回整包长度
{
	if(b[0]!=address && b[0]!=0) //若不是自己该接收的
	{
		return 6;
	}
	if(b[1]==0x10) //若是写配置，则长度不一定
	{
		if(b[6]>MAX_REG_NUM*2) //限制数据长度30字节，根据需要取消
		{
			return 6; //长度错误
		}
		return b[6]+9;
	}
	else
	{
		return 8;
	}
}
s64 CModbus_Slave::pro_pack(u8 * p,s64 len) //从机接收处理
{
	int err=0;
	int i;
	u16 crc=0;
	u8 send_len=0; //发送字节数
	if(len<8) //长度错误
	{
		return 1;
	}
	crc=GetModbusCRC16(p,len-2);
	if(crc!=*(u16*)(p+len-2))
	{
		return 1;
	}
	if(PACK.addr!=0&&PACK.addr!=address)
		return 0;
	if(p[1]==4 || p[1]==3) //读输入
	{
		u16 start=((u16*)p)[1]; //起始地址
		u8 n=p[5]; //参数个数
		start=CHANGE_END16(start);
		if(n>MAX_REG_NUM)
		{
			err=1;
			n=0;
		}
	//回复
		tx_buf[0]=address;
		tx_buf[1]=p[1];
		tx_buf[2]=n*2; //个数,单位字节,是数据域的字节数
		for(i=0;i<n;i++)
		{
			u16 *pd=get_data(start+i);
			if(pd==0) //没有找到寄存器
			{//有一个地址没找到，就直接返回
				send_len=send_err(p[1],1);
				goto END_SEND;
			}
			else
			{
				((u16*)(tx_buf+3))[i]=CHANGE_END16(*pd);
			}
		}
		//从串口发送出去
		send_len=tx_buf[2]+3+2;
	}
	else if(p[1]==0x06) //写单寄存器
	{
		u16 start=PACK.st; //起始地址
		u16 *pd=0;
		start=CHANGE_END16(start);
		pd=get_data(start);
		TP=PACK;
		if(pd==0) //没有找到寄存器
		{//有一个地址没找到，就直接返回
			send_err(p[1],1);
			goto END_SEND;
		}
		else
		{
			u16 t=PACK.num; //数据值
			t=CHANGE_END16(t);
			*pd=t;
		}
		TP.addr=address;
		//从串口发送出去
		send_len=8;
	}
	else if(p[1]==0x10) //多个写入
	{
		//获取写入数据
		u8 num=p[5]; //写入个数
		u8 l=p[6]; //数据长度
		u16 addr=((u16*)p)[1];
		if(num*2!=l) return 3;
		addr=CHANGE_END16(addr);
		for(i = 0; i < num; i++)
		{
			u16 *pd=get_data(addr);
			addr+=1;
			if(pd==0) //没有找到寄存器
			{//有一个地址没找到，就直接返回
				send_len=send_err(p[1],1);
				goto END_SEND;
			}
			else
			{
				u16 t=((u16*)(p+7))[i]; //数据值
				*pd=CHANGE_END16(t);
			}
		}
	//回复
		tx_buf[0]=address;
		tx_buf[1]=p[1];
		((u16*)tx_buf)[1]=((u16*)p)[1]; //起始地址(都是大端不用转)
		((u16*)tx_buf)[2]=((u16*)p)[2]; //寄存器个数(都是大端不用转)
		//从串口发送出去
		send_len=8;
	}
	else //不支持的功能
	{
		send_len=send_err(p[1],8);
	}
END_SEND:
	if(send_len>2 && PACK.addr!=0) //若不是广播
	{
		*(u16*)(tx_buf+send_len-2)=GetModbusCRC16(tx_buf,send_len-2);
		send_fun(tx_buf,send_len);
	}
	rx_fun(p,len);
	return 0;
}
u8 CModbus_Slave::send_err(u8 cmd,u8 err)
{
	u16 crc;
	TP.addr=address;
	TP.fun=cmd | 0x80;
	tx_buf[2]=err;
	return 5;
}
////////////////////////////////////////////////////////////////////////////

