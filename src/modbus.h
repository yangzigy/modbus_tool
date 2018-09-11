/*
文件名：modbus.h
时间：2017-7-18
功能：

*/
#ifndef MODBUS_H
#define MODBUS_H

#include "main.h"
#include "comm_pack.h"

//实现的帧：全部大端存储
//04：读输入寄存器，8字节
//03：读保持寄存器，8字节(一般使用)
//03/04：回复，01 04 字节数 数据 crc
//06：写单寄存器，8字节
//06回复：01 06 地址 数据 crc  共8字节
//10：写配置：01 10 起始地址(2) 数据个数(2) 字节数(1) 数据 crc
//10：回复：01 10 起始地址(2) 数据个数(2) crc
//异常：地址+功能码（最高位置1）+错误码+校验
//0 无错误
//1 内存范围错误
//2 非法波特率或校验
//3 非法从属地址
//4 非法Modbus参数值
//5 保持寄存器与Modbus从属符号重叠
//6 收到校验错误
//7 收到CRC错误
//8 非法功能请求／功能不受支持
//9 请求中的非法内存地址
//10 从属功能未启用

//主机接收的数据包：
//	03/04回复包：偏移2时接收到字节数
//	06/10回复包：固定为8字节
//	异常包：固定5字节
//从机接收数据包：
//	03/04/06请求包：8字节
//	10包：偏移6确定字节数,最小11字节
//	其他子机的数据包，同主机
#pragma pack(1)
typedef struct
{
	u8 addr;
	u8 fun;
	u16 st;
	u16 num;
	u16 crc;
} MODBUS_RTU_REQ;//modbus rtu的查询帧
#pragma pack()

typedef struct _tag_MODBUS_ADDR_LIST//modbus地址列表
{ //作为从机的寄存器列表
	u16 st; //起始地址
	u16 num;//此数组中地址个数
	u16 *buf; //数据缓存
	struct _tag_MODBUS_ADDR_LIST *next;
/////////////////////////////////
//作为主机的任务列表
	u8 addr; //地址
	u8 type; //任务类型(04 03)
	u8 freq; //任务执行频率,tick数量，若为0，则为单次任务
	u8 tick; //计时,1开始，单次触发任务需要写2
	u8 err; //0无错，1无回应
	u8 stat; //0空闲，1正在发送，2正确回复，3错误回复
} MODBUS_ADDR_LIST; //也用作任务列表

#define MAX_REG_NUM		123 //最大单次请求寄存器数量

void modbus_send_void(u8 *p,int n);
class CModbus : public CComm_Pack
{
public:
	u8 headbuf[256];
	u8 address; //modbus地址
	u8 tx_buf[256];
	void (*send_fun)(u8 *p,int n);//发送函数
	void (*rx_fun)(u8 *p,int n);//接收回调函数
	MODBUS_ADDR_LIST *addr_list; //主机任务列表，或从机地址列表
	CModbus()
	{
		address=1;
		send_fun=modbus_send_void;
		rx_fun=modbus_send_void;
		addr_list=0;

		rec_buff=headbuf;
		buf_len=sizeof(headbuf);
		//SYNC=&address;
		//syncbuf_len=1;
		SYNC=0;
		syncbuf_len=0;
		pack_len=8;
		pre_offset=3;
		pre_p=0;
	}
	void reg(MODBUS_ADDR_LIST *a); //向模块注册周期任务，或地址
	u16 *get_data(u16 a); //从地址获得数据,输入小端地址
};
//主：
//发送数据，可以是03、04、06、10
//接收数据：存在数据缓存中
class CModbus_Master : public CModbus
{
public:
	CModbus_Master() : CModbus()
	{
		pre_offset=3;
	}
	MODBUS_ADDR_LIST *cur_send; //当前等待的任务
	virtual s64 pre_pack_len(u8 *b,s64 len);//返回整包长度
	virtual s64 pro_pack(u8 * b,s64 len);//返回是否正确接收
	int host_send(u8 addr,u8 fun,u16 st,u16 num,u16 *d); //当发06时，num为寄存器值
	void poll(void); //周期函数，主机通过周期函数进行发送
};
//从：
//从数据列表中取得数据，回复给主。
//写入的数据直接写入寄存器地址中
class CModbus_Slave : public CModbus
{
public:
	CModbus_Slave() : CModbus()
	{
		pre_offset=7;
	}
	virtual s64 pre_pack_len(u8 *b,s64 len);//返回整包长度
	virtual s64 pro_pack(u8 * b,s64 len);//返回是否正确接收
	u8 send_err(u8 cmd,u8 err);
};

u16 GetModbusCRC16(u8 *cp,int leng);

#endif

