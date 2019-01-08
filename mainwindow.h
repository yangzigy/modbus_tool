#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QLayout>
#include <QMouseEvent>
#include <QtCharts>
#include "qlabel.h"
#include "qserialport.h"
#include "qserialportinfo.h"
#include "json.h"
#include "common.h"
#include "appflow.h"

extern Json::Value config; //配置对象
extern CModbus_Master md_master;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	Ui::MainWindow *ui;
	void ui_initial();
	void updateUI_regs(void); //从数据更新界面：寄存器列表
	void updateUI_tasks(void); //从数据更新界面：任务列表
	void updateData_regs(void); //从界面更新数据：寄存器列表
	void updateData_tasks(void); //从界面更新数据：任务列表
////////////////////////////////////////////////////////////////
// 基本定义
////////////////////////////////////////////////////////////////
	QSerialPort *uart;
	int timerid;
	virtual void timerEvent(QTimerEvent *event);
	virtual void closeEvent(QCloseEvent *event);

	QChartView *chartView0;
	QChart *chart0;
signals:
	void uart_rxpro_signal(void); //uart接收处理
public slots:
	void slot_uart_rx(); //串口接收
	void uart_rxpro_slot(int type); //uart接收处理
	void on_bt_open_uart_clicked();
private slots:
	void on_bt_start_task_clicked();
	void on_bt_help_clicked();
	void on_bt_add_task_clicked();
	void on_bt_add_reg_clicked();
	void on_bt_del_task_clicked();
	void on_bt_clear_data_clicked();
	void on_bt_del_reg_clicked();
};

#endif // MAINWINDOW_H

