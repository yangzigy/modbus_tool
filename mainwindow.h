#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QString>
#include <QFileDialog>
#include <QLayout>
#include <QMouseEvent>
#include <QtCharts>
#include "qlabel.h"
#include "qserialport.h"
#include "qserialportinfo.h"
#include "json.h"
#include "common.h"
#include "modbus.h"

extern Json::Value config; //配置对象
extern CModbus_Master md_master;

namespace Ui {
class MainWindow;
}

class CModbus_RegDis; //modbus寄存器显示
class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	void ui_initial();
////////////////////////////////////////////////////////////////
// 基本定义
////////////////////////////////////////////////////////////////
	QSerialPort *uart;
	int timerid;
	virtual void timerEvent(QTimerEvent *event);
	virtual void closeEvent(QCloseEvent *event);

	QChartView *chartView0;
	QChart *chart0;
	vector<shared_ptr<CModbus_RegDis> > reg_list;
signals:
	void uart_rxpro_signal(void); //uart接收处理

public slots:
	void slot_uart_rx(); //串口接收
	void uart_rxpro_slot(int type); //uart接收处理

	void on_bt_open_uart_clicked();
private:
	Ui::MainWindow *ui;
};

class CModbus_RegDis //modbus寄存器显示
{
public:
	CModbus_RegDis(){}
	string name=""; //寄存器名称
	u8 addr=0; //地址
	u16 reg=0; //寄存器地址
	int data_type=0; //显示类型,0:原始码，1:u16,2:s16,3:u8,4:s8
	float data_factor=1;
	float data_offset=0;
	int display_type=0; //0:十进制,1:HEX,2:float
};
class CModbus_TaskList //modbus通信对象
{
public:
	CModbus_RegComm(){}
	string name=""; //通信对象名
	MODBUS_ADDR_LIST tl; //任务列表
};

#endif // MAINWINDOW_H

