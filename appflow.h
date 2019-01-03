#ifndef APPFLOW_H
#define APPFLOW_H

#include "common.h"
#include "json.h"
#include "modbus.h"

class CMReg //modbus寄存器定义
{
public:
	CMReg(){}
	string name=""; //寄存器名称
	u8 addr=1; //从机地址
	u16 reg=0; //寄存器地址
	u8 is_curv=1; //是否加入曲线显示
	u8 d_type=0; //显示类型,0:u16,1:s16,2:u8,3:s8,4:u16-float,5:s16-float,6:u8-float,7:s8-float
	double d_k=1; //数据放大系数
	double d_off=0; //数据偏移
	Json::Value toJson(void)
	{
		Json::Value v;
		v["name"]=name;
		v["addr"]=addr;
		v["reg"]=reg;
		v["is_curv"]=is_curv;
		v["d_type"]=d_type;
		v["d_k"]=d_k;
		v["d_off"]=d_off;
		return v;
	}
	int fromJson(Json::Value &v)
	{
		if(v["name"].isString() && 
			v["reg"].isInt() && 
			v["is_curv"].isInt() && 
			v["addr"].isInt())
		{
			name=v["name"].asString();
			reg=v["reg"].asInt();
			is_curv=v["is_curv"].asInt();
			addr=v["addr"].asInt();
			d_type=v.get("d_type",d_type).asInt();
			jsonget(v,"d_k",d_k);
			jsonget(v,"d_off",d_off);
			return 0;
		}
		return 1;
	}
	u16 dbuf=0; //寄存器的内存空间
};
class CMTask //modbus周期任务
{
public:
	CMTask(){}
	string name=""; //寄存器名称
	u8 addr=1; //从机地址
	u8 type=3; //类型：06 10 03 04
	u16 reg=0; //寄存器地址
	u16 num=1; //数量
	float fre=1; //执行频率
	Json::Value toJson(void)
	{
		Json::Value v;
		v["name"]=name;
		v["addr"]=addr;
		v["reg"]=reg;
		v["type"]=type;
		v["num"]=num;
		v["fre"]=fre;
		return v;
	}
	int fromJson(Json::Value &v)
	{
		if(v["name"].isString() && 
			v["reg"].isInt() && 
			v["type"].isInt() && 
			v["num"].isInt() && 
			v["fre"].isDouble() && 
			v["addr"].isInt())
		{
			name=v["name"].asString();
			reg=v["reg"].asInt();
			type=v["type"].asInt();
			num=v["num"].asInt();
			fre=v["fre"].asDouble();
			addr=v["addr"].asInt();
			return 0;
		}
		if(pbuf) delete pbuf;
		pbuf=new u16[num];
		return 1;
	}
	u16 *pbuf=0; //任务数据缓存
};

extern vector<CMReg> regs_list; //寄存器列表
extern vector<CMTask> task_list; //任务列表
extern CModbus_Master main_md;

int app_ini(Json::Value v); //将配置文件读入内存列表中
void task_start(void); //开始任务,将任务列表中的任务变成modbus模块的任务
void task_stop(void); //结束任务
void task_poll(void); //任务周期函数，100Hz

#endif

