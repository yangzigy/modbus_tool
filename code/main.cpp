#include "mainwindow.h"
#include <QApplication>
#include "common.h"
#include "ui_mainwindow.h"

MainWindow *pw=0;
Json::Value config; //配置对象

void modbus_send_uart(u8 *p,int n) //modbus模块发送
{
	if(pw->uart->isOpen())
	{
		pw->uart->write((const char *)p,n);
		if(is_master) pw->ui->te_comm_log->tx_pack(p,n); //加入日志
		else  pw->ui->te_comm_log->tx_slave_pack(p,n); //加入日志
	}
	else QMessageBox::information(pw,"错误","串口未打开");
}
void modbus_lostlock(u8 *p,int n) //modbus模块失锁
{
	for(int i=0;i<n;i++)
	{
		pw->signal_modbus_lostlock(p[i]); //加入日志
	}
}
void modbus_rxpack(u8 *p,int n) //modbus模块接收完整帧
{
	pw->signal_modbus_rxpack(p,n); //加入日志
}
void update_a_reg(u8 addr,u16 reg,u16 d) //更新一个寄存器
{
	pw->signal_update_a_reg(addr,reg,d);
}
///////////////////////////////////////////////////////////////////////
//				入口
///////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	main_md.send_fun=modbus_send_uart;
	main_md.lostlock_fun=modbus_lostlock;
	slave_md.send_fun=modbus_send_uart;
	slave_md.lostlock_fun=modbus_lostlock;

    QApplication a(argc, argv);
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//配置
	start_program();
	string name=exepath+"/cfg.txt";
	string text=read_textfile(name.c_str());
	Json::Reader reader;
	reader.parse(text.c_str(),config,false); //可以有注释,false不会复制
	pjson(config); //输出配置字符
	MainWindow w;
	w.show();
	pw=&w;
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//初始化
	app_ini(config);
	pw->ui_initial();
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//主循环
    return a.exec();
}

CComFile flog;
CDateTime date;
void rec_log(const char *ps) //记录日志或发出传感数据
{
	if(flog.f)
	{
		int n=strlen(ps);
		flog.write((void*)ps,n);
		fflush(flog.f);
	}
	else
	{
		string name=date.Now("%Y%m%d_%H%M%S")+".txt";
		flog.open(name.c_str(),"w");
	}
}
