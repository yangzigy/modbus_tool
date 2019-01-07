#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>

const u32 com_baud_tab[]=//串口波特率表
{
	1200,2400,4800,9600,19200,28800,38400,57600,115200,
	230400,460800,500000,576000,921600,
};

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
	{
		ui->cb_uart->addItem(info.portName());
	}
	for(auto &it:com_baud_tab)
	{
		ui->cb_baud->addItem(QString().sprintf("%d",it));
	}
	ui->cb_baud->setCurrentIndex(8);

	uart=new QSerialPort();
	QObject::connect(uart, SIGNAL(readyRead()), this, SLOT(slot_uart_rx()));
	QObject::connect(this,SIGNAL(uart_rxpro_signal(int)),this,SLOT(uart_rxpro_slot(int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::ui_initial()
{
	ui->tw_regs->setColumnCount(9);
	ui->tw_regs->setRowCount(regs_list.size());
	ui->tw_regs->setHorizontalHeaderLabels(QStringList() <<
		"名称"<<"原始值"<<"值"<<"地址"<<"寄存器"<<"曲线"<<"类型"<<"系数"<<"偏移");
	ui->tw_regs->setColumnWidth(0,60);
	ui->tw_regs->setColumnWidth(1,60);
	ui->tw_regs->setColumnWidth(2,60);
	ui->tw_regs->setColumnWidth(3,40);
	ui->tw_regs->setColumnWidth(4,50);
	ui->tw_regs->setColumnWidth(5,35);
	ui->tw_regs->setColumnWidth(6,50);
	ui->tw_regs->setColumnWidth(7,50);
	ui->tw_regs->setColumnWidth(8,50);
	ui->tw_tasks->setColumnCount(6);
	ui->tw_tasks->setRowCount(regs_list.size());
	ui->tw_tasks->setHorizontalHeaderLabels(QStringList() <<
		"名称" << "地址" << "寄存器" << "类型"<<"数量"<<"状态");
	ui->tw_tasks->setColumnWidth(0,60);
	ui->tw_tasks->setColumnWidth(1,40);
	ui->tw_tasks->setColumnWidth(2,50);
	ui->tw_tasks->setColumnWidth(3,40);
	ui->tw_tasks->setColumnWidth(4,40);
	ui->tw_tasks->setColumnWidth(5,60);
	QStringList regtype_list; //寄存器类型列表
    regtype_list.append("u16");
    regtype_list.append("s16");
    regtype_list.append("u8");
    regtype_list.append("s8");
    regtype_list.append("u16f");
    regtype_list.append("s16f");
    regtype_list.append("u8f");
    regtype_list.append("s8f");
	QStringList tasktype_list; //寄存器类型列表
    tasktype_list.append("06");
    tasktype_list.append("10");
    tasktype_list.append("03");
    tasktype_list.append("04");

	for(int i=0;i<regs_list.size();i++) //遍历所有的寄存器
	{
		QTableWidgetItem *item;
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 0, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 1, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 2, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 3, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 4, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 5, item);
		//item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 6, item);
		QComboBox *tmpcombo=new QComboBox(); tmpcombo->addItems(regtype_list);
		ui->tw_regs->setCellWidget(i,6,tmpcombo);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 7, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 8, item);

		ui->tw_regs->item(i, 0)->setText(regs_list[i].name.c_str());
		ui->tw_regs->item(i, 1)->setText(QString().sprintf("%04X",regs_list[i].dbuf));
		ui->tw_regs->item(i, 2)->setText(QString().sprintf("%d",regs_list[i].dbuf));
		ui->tw_regs->item(i, 3)->setText(QString().sprintf("%d",regs_list[i].addr));
		ui->tw_regs->item(i, 4)->setText(QString().sprintf("%d",regs_list[i].reg));
		ui->tw_regs->item(i, 5)->setCheckState(regs_list[i].is_curv?Qt::Checked : Qt::Unchecked);
		((QComboBox *)(ui->tw_regs->cellWidget(i, 6)))->setCurrentIndex(regs_list[i].d_type);
		ui->tw_regs->item(i, 7)->setText(QString().sprintf("%.2f",regs_list[i].d_k));
		ui->tw_regs->item(i, 8)->setText(QString().sprintf("%.2f",regs_list[i].d_off));
	}
	for(int i=0;i<task_list.size();i++) //遍历所有任务
	{
		QTableWidgetItem *item;
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 0, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 1, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 2, item);
		//item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 3, item);
		QComboBox *tmpcombo=new QComboBox(); tmpcombo->addItems(tasktype_list);
		ui->tw_tasks->setCellWidget(i,3,tmpcombo);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 4, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 5, item);

		ui->tw_tasks->item(i, 0)->setText(task_list[i].name.c_str());
		ui->tw_tasks->item(i, 1)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.addr));
		ui->tw_tasks->item(i, 2)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.st));
		((QComboBox *)(ui->tw_tasks->cellWidget(i, 3)))->setCurrentIndex(task_list[i].mdbs_buf.type);
		ui->tw_tasks->item(i, 4)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.num));
		ui->tw_tasks->item(i, 5)->setText("");
	}
	chart0 = new QChart();
	QMargins tmpmarg(5,5,5,5);
	chart0->setMargins(tmpmarg);
	chartView0 = new QChartView(chart0);

	timerid=startTimer(10); //初始化定时器

	ui->te_comm_log->setReadOnly(true);
	u8 tb[8]={1,3,0,8,0,2,0xcc,0xba};
	u8 tb10[]={1,0x10,0,8,0,2,4,0,1,0,2,0xcc,0xba};
	u8 rb03[]={1,3,2,8,0,2,4,0xcc,0xba};
	u8 rberr[]={1,3,2,0xcc,0xba};
	ui->te_comm_log->tx_pack(tb,sizeof(tb));
	ui->te_comm_log->rx_pack(rb03,sizeof(rb03));
	ui->te_comm_log->tx_pack(tb10,sizeof(tb10));
	ui->te_comm_log->rx_lostlock(rb03,sizeof(rb03));
	ui->te_comm_log->rx_pack(rberr,sizeof(rberr));
}
void MainWindow::timerEvent(QTimerEvent *event) //100Hz
{
	if(event->timerId() == timerid) //定时器调用，或触发
	{
		task_poll();
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
}

void MainWindow::slot_uart_rx() //串口接收
{
	char buf[128];
	int n=1;
	while(n)
	{
		n=uart->read(buf,sizeof(buf));
		main_md.pack((u8*)buf,n);
	}
}
void MainWindow::uart_rxpro_slot(int type) //uart接收处理
{
	if(type==1) //控制状态包
	{
	}
}

/////////////////////////////////////////////////////////////////////////
//界面响应
void MainWindow::on_bt_open_uart_clicked()
{
	if(ui->bt_open_uart->text()=="打开串口")
	{
		uart->setPortName(ui->cb_uart->currentText());
		uart->setBaudRate(ui->cb_baud->currentText().toInt());
		if(uart->open(QIODevice::ReadWrite))
		{
			ui->bt_open_uart->setText("关闭串口");
		}
	}
	else
	{
		uart->close();
		ui->bt_open_uart->setText("打开串口");
	}
}

