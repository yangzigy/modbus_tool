#include "appflow.h"

//////////////////////////////////////////////////////////////////////////////////
//				业务数据
//////////////////////////////////////////////////////////////////////////////////
vector<CMReg> regs_list; //寄存器列表
vector<CMTask> task_list; //任务列表

//运行对象
static MODBUS_ADDR_LIST *md_q_buf[10]; //最多几个任务等待
CModbus_Master main_md(md_q_buf,sizeof(md_q_buf)/sizeof(MODBUS_ADDR_LIST*));
CModbus_Slave slave_md;
int is_master=1; //是否是主模式
int is_running=0; //是否正在运行
u16 freq_2_tick(float f) //频率转换成间隔
{
	return f>=0.01?100/f:100;
}
float tick_2_freq(u16 tick) //间隔转换成频率
{
	return tick>0?(100.0f/tick):0;
}
#define MCS		(*(main_md.cur_task))
void mdbs_rxcb(u8 *p,int n)//接收回调函数
{
	//接收回调进入日志
	modbus_rxpack(p,n);
	//判断是否是读
	if(main_md.cur_task==0) return ;
	if(MCS.type==4 || MCS.type==3)
	{
		//拿到当前任务，将任务缓存中的数据更新
		for(int i=0;i<MCS.num;i++) //每一个寄存器
		{
			update_a_reg(MCS.addr,MCS.st+i,MCS.buf[i]);
		}
	}
}
void mdbs_rxcb_slave(u8 *p,int n)//接收回调函数
{
	//接收回调进入日志
	modbus_rxpack(p,n);
	if(p[1]==6)
	{
		u16 reg=(*(MODBUS_RTU_REQ*)p).st;
		u16 d=(*(MODBUS_RTU_REQ*)p).num;
		d=CHANGE_END16(d);
		update_a_reg(p[0],reg,d);
	}
	if(p[1]==0x10)
	{
		int i;
		u16 reg=(*(MODBUS_RTU_REQ*)p).st;
		u16 num=(*(MODBUS_RTU_REQ*)p).num;
		num=CHANGE_END16(num);
		for(i=0;i<num;i++)
		{
			u16 d=((u16*)(p+7))[i];
			d=CHANGE_END16(d);
			update_a_reg(p[0],reg+i,d);
		}
	}
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
	task_list.clear();
	for(auto &it:v["tasklist"])
	{
		CMTask tt;
		if(tt.fromJson(it)) continue;
		task_list.push_back(tt);
	}
	main_md.rx_fun=mdbs_rxcb;
	slave_md.rx_fun=mdbs_rxcb_slave;
}
void app_master_slave(int m) //切换主从模式
{
}
//////////////////////////////////////////////////////////////////////////////////
//				任务控制
//////////////////////////////////////////////////////////////////////////////////
void task_start(void) //开始任务,将任务列表中的任务变成modbus模块的任务
{
	//开始执行
	is_running=1;
}
void task_stop(void) //结束任务
{
	is_running=0; //结束指令
}
void task_poll(void) //任务周期函数，100Hz
{
	main_md.poll();
	if(is_running==1) //若在运行状态
	{
		for(auto &it:task_list)
		{
			if(it.enable==0) continue;
			it.tick++;
			if(it.tick>=freq_2_tick(it.freq)) //若到了该发送的时间
			{
				it.tick=0;
				main_md.add_task(&(it.mdbs_buf)); //由于界面保证了执行任务时任务列表不变,所以可以引用
			}
		}
	}
}

