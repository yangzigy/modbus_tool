#include "mainwindow.h"
#include <QApplication>
#include "common.h"
#include "ui_mainwindow.h"

MainWindow *pw=0;
Json::Value config; //配置对象

void modbus_send_uart(u8 *p,int n)
{
	if(pw->uart->isOpen())
	{
		pw->uart->write((const char *)p,n);
		pw->ui->te_comm_log->tx_pack(p,n); //加入日志
	}
}
///////////////////////////////////////////////////////////////////////
//				入口
///////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	main_md.send_fun=modbus_send_uart;

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

