#include "mainwindow.h"
#include <QApplication>
#include "common.h"

MainWindow *pw=0;

Json::Value config; //配置对象
///////////////////////////////////////////////////////////////////////
//				入口
///////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
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

