#include "appflow.h"

//////////////////////////////////////////////////////////////////////////////////
//				业务数据
//////////////////////////////////////////////////////////////////////////////////
vector<CMReg> regs_list; //寄存器列表
vector<CMTask> task_list; //任务列表

//运行对象
CModbus_Master main_md;
int is_running=0; //是否正在运行
u16 freq_2_tick(float f) //频率转换成间隔
{
	return f>=0.01?100/f:100;
}
float tick_2_freq(u16 tick) //间隔转换成频率
{
	return tick>0?(100.0f/tick):0;
}
//////////////////////////////////////////////////////////////////////////////////
//				初始化
//////////////////////////////////////////////////////////////////////////////////
int app_ini(Json::Value v) //将配置文件读入内存列表中
{
	if((!v.isObject()) || (!v["datalist"].isArray()) ||
		(!v["tasklist"].isArray())) return 1;
	regs_list.clear();
	for(auto &it:v["datalist"])
	{
		CMReg tr;
		if(tr.fromJson(it)) continue;
		regs_list.push_back(tr);
	}
	for(auto &it:v["tasklist"])
	{
		CMTask tt;
		if(tt.fromJson(it)) continue;
		task_list.push_back(tt);
	}
}
//////////////////////////////////////////////////////////////////////////////////
//				任务控制
//////////////////////////////////////////////////////////////////////////////////
void task_start(void) //开始任务,将任务列表中的任务变成modbus模块的任务
{
	//将任务列表中的任务变成modbus模块的任务
	if(is_running || main_md.addr_list) //若开始前还没清理干净，就让关闭函数清理一遍
	{
		is_running=2;
		return ;
	}
	for(auto &it:task_list) //现在main_md的addr_list是空的
	{
		main_md.reg(&(it.mdbs_buf)); //主机注册任务
	}
	//开始执行
	is_running=1;
}
void task_stop(void) //结束任务
{
	is_running=2; //结束指令
}
void task_poll(void) //任务周期函数，100Hz
{
	if(is_running==2)
	{
		is_running=0;
		//调用已经关闭任务的处理
		//这是下一次调用，回复已经超时
		main_md.addr_list=0;
	}
	else if(is_running==1) //若在运行状态
	{
		main_md.poll();
	}
	else if(is_running==3) //单次任务
	{
		main_md.poll();
		is_running=2;
	}
}
