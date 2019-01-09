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
	//增删时全部重做：
	void regs_create_UI(void); //从数据更新界面：寄存器列表
	void tasks_create_UI(void); //从数据更新界面：任务列表
	//建立时调用、后台数据变化时
	void regs_update_UI_row(int row); //刷新界面：寄存器
	void tasks_update_UI_row(int row); //刷新界面：任务
	void regs_update_UI(void); //刷新界面：寄存器
	void tasks_update_UI(void); //刷新界面：任务
	//3Hz周期刷新
	void regs_update_data(void); //从界面更新数据：寄存器列表
	void tasks_update_data(void); //从界面更新数据：任务列表
////////////////////////////////////////////////////////////////
// 基本定义
////////////////////////////////////////////////////////////////
	QSerialPort *uart;
	int timerid;
	virtual void timerEvent(QTimerEvent *event);
	virtual void closeEvent(QCloseEvent *event);
////////////////////////////////////////////////////////////////
// 曲线
////////////////////////////////////////////////////////////////
	QChartView *chartView0;
	QChart *chart0;
	map<int,QLineSeries *> curv_map; //曲线列表
	u32 sttime=0;
	int is_auto_fitscreen=1; //自动自适应屏幕
signals:
	void uart_rxpro_signal(void); //uart接收处理
	void signal_modbus_lostlock(u8 *p,int n); //modbus模块失锁
	void signal_update_a_reg(u8 addr,u16 reg,u16 d); //更新一个寄存器
	void signal_modbus_rxpack(u8 *p,int n); //modbus模块接收完整帧
public slots:
	void slot_uart_rx(); //串口接收
	void slot_modbus_lostlock(u8 *p,int n); //modbus模块失锁
	void slot_update_a_reg(u8 addr,u16 reg,u16 d); //更新一个寄存器
	void slot_modbus_rxpack(u8 *p,int n); //modbus模块接收完整帧
private slots:
	void on_bt_open_uart_clicked();
	void on_bt_start_task_clicked();
	void on_bt_help_clicked();
	void on_bt_add_task_clicked();
	void on_bt_fitscreen_clicked();
	void on_bt_add_reg_clicked();
	void on_bt_del_task_clicked();
	void on_bt_clear_data_clicked();
	void on_bt_del_reg_clicked();
};

#endif // MAINWINDOW_H

