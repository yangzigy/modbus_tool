#include "mainwindow.h"
#include <QApplication>
#include "common.h"

MainWindow *pw=0;
Json::Value config; //配置对象

CModbus_Master md_master;
/*
MODBUS_ADDR_LIST md_task_read= //主机任务
{//起始地址,此数组中地址个数,数据缓存,next
	3,19,(u16*)&(md_reg_r.IO),0,
//地址,任务类型,任务执行频率,计时,err,stat
	1,4,10, 0,0,0,
};

	main_md.reg(&md_task_read); //主机注册任务

*/
void modbus_send_uart(u8 *p,int n)
{
	if(pw->uart->isOpen())
	{
		pw->uart->write((const char *)p,n);
	}
}
///////////////////////////////////////////////////////////////////////
//				入口
///////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	md_master.send_fun=modbus_send_uart;

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
//UI逻辑初始化
	pw->ui_initial();
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//主循环
    return a.exec();
}

