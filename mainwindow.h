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

extern Json::Value config; //配置对象

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
	string name; //寄存器名称
	u16 addr; //寄存器地址
	int type; //显示类型,0:原始码，1:0.1,2:0.01,3:0.001,4:0.0001,5:0.00001,6~11为有符号数
	int rw; //0读1写
};


#endif // MAINWINDOW_H
