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
	//读配置文件，取得多少个写寄存器，多少个读寄存器
	for(auto &it:config)
	{
		CModbus_RegDis *pt=new CModbus_RegDis();
		pt->addr=it.get("addr",0).asInt();
		pt->name=it.get("name",QString().sprintf("%04X",pt->addr).toStdString().c_str()).asString();
		pt->type=it.get("type",0).asInt();
		pt->rw=it.get("rw",0).asInt();
	}
	chart0 = new QChart();
	QMargins tmpmarg(5,5,5,5);
	chart0->setMargins(tmpmarg);
	chartView0 = new QChartView(chart0);

	timerid=startTimer(100); //初始化定时器
}
void MainWindow::timerEvent(QTimerEvent *event) //10Hz
{
	if(event->timerId() == timerid) //定时器调用，或触发
	{
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

