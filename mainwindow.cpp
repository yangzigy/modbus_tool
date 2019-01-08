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
	updateUI_regs();
	updateUI_tasks();

	chart0 = new QChart();
	QMargins tmpmarg(5,5,5,5);
	chart0->setMargins(tmpmarg);
	chartView0 = new QChartView(chart0);

	timerid=startTimer(10); //初始化定时器

	ui->te_comm_log->setReadOnly(true);
	//测试
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
void MainWindow::updateUI_regs(void) //从数据更新界面：寄存器列表
{
	ui->tw_regs->clear();
	ui->tw_regs->setColumnCount(9);
	ui->tw_regs->setRowCount(regs_list.size());
	ui->tw_regs->setHorizontalHeaderLabels(QStringList() <<
		"名称"<<"原始值"<<"值"<<"地址"<<"寄存器"<<"曲线"<<"类型"<<"系数"<<"偏移");
	ui->tw_regs->setColumnWidth(0,60);
	ui->tw_regs->setColumnWidth(1,60);
	ui->tw_regs->setColumnWidth(2,70);
	ui->tw_regs->setColumnWidth(3,40);
	ui->tw_regs->setColumnWidth(4,50);
	ui->tw_regs->setColumnWidth(5,35);
	ui->tw_regs->setColumnWidth(6,50);
	ui->tw_regs->setColumnWidth(7,50);
	ui->tw_regs->setColumnWidth(8,50);
	QStringList regtype_list; //寄存器类型列表
    regtype_list.append("u16");
    regtype_list.append("s16");
    regtype_list.append("u8");
    regtype_list.append("s8");
    regtype_list.append("u16f");
    regtype_list.append("s16f");
    regtype_list.append("u8f");
    regtype_list.append("s8f");

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
		ui->tw_regs->item(i, 2)->setText(QString().sprintf("%f",regs_list[i].org_2_val(regs_list[i].dbuf)));
		ui->tw_regs->item(i, 3)->setText(QString().sprintf("%d",regs_list[i].addr));
		ui->tw_regs->item(i, 4)->setText(QString().sprintf("%d",regs_list[i].reg));
		ui->tw_regs->item(i, 5)->setCheckState(regs_list[i].is_curv?Qt::Checked : Qt::Unchecked);
		((QComboBox *)(ui->tw_regs->cellWidget(i, 6)))->setCurrentIndex(regs_list[i].d_type);
		ui->tw_regs->item(i, 7)->setText(QString().sprintf("%.2f",regs_list[i].d_k));
		ui->tw_regs->item(i, 8)->setText(QString().sprintf("%.2f",regs_list[i].d_off));
	}
}
void MainWindow::updateData_regs(void) //从界面更新数据：寄存器列表
{
	int row=ui->tw_regs->rowCount();
	int col=ui->tw_regs->columnCount();
	for(int i=0;i<row;i++) //对于每一行
	{
		bool b;
		regs_list[i].name=ui->tw_regs->item(i, 0)->text().toStdString();
		float fd_k=ui->tw_regs->item(i, 7)->text().toFloat();
		float fd_off=ui->tw_regs->item(i, 8)->text().toFloat();
		if(fabs(fd_k-regs_list[i].d_k)>0.00001 ||
			fabs(fd_off-regs_list[i].d_off)>0.00001) //若放大倍数/偏移有变化
		{
			regs_list[i].d_k=fd_k;
			regs_list[i].d_off=fd_off;
			ui->tw_regs->item(i, 2)->setText(QString().sprintf("%f",regs_list[i].org_2_val(regs_list[i].dbuf)));
			return ; //否则会干扰后边
		}
		regs_list[i].d_k=fd_k;
		regs_list[i].d_off=fd_off;
		u16 td_org=ui->tw_regs->item(i, 1)->text().toInt(&b,16); //原始数值
		u16 td_val=regs_list[i].val_2_org(ui->tw_regs->item(i, 2)->text().toFloat()); //值换算为原始数值
		if(regs_list[i].dbuf != td_org) //原始数值的显示发生了变化
		{
			regs_list[i].dbuf=td_org;
			ui->tw_regs->item(i, 2)->setText(QString().sprintf("%f",regs_list[i].org_2_val(regs_list[i].dbuf)));
		}
		else if(regs_list[i].dbuf != td_val) //若显示值被修改
		{
			regs_list[i].dbuf=td_val;
			ui->tw_regs->item(i, 1)->setText(QString().sprintf("%04X",regs_list[i].dbuf));
		}
		regs_list[i].reg=ui->tw_regs->item(i, 4)->text().toInt();
		regs_list[i].is_curv=ui->tw_regs->item(i, 5)->checkState()?1:0;
		regs_list[i].d_type=((QComboBox *)(ui->tw_regs->cellWidget(i, 6)))->currentIndex();
	}
}
void MainWindow::updateUI_tasks(void) //从数据更新界面：任务列表
{
	QObject::disconnect(ui->tw_tasks,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(on_tw_tasks_itemChanged(QTableWidgetItem*)));
	ui->tw_tasks->clear();
	ui->tw_tasks->setColumnCount(8);
	ui->tw_tasks->setRowCount(task_list.size());
	ui->tw_tasks->setHorizontalHeaderLabels(QStringList() <<
		"使能"<<"名称"<<"地址"<<"寄存器"<<"类型"<<"数量"<<"频率"<<"状态");
	ui->tw_tasks->setColumnWidth(0,34);
	ui->tw_tasks->setColumnWidth(1,60);
	ui->tw_tasks->setColumnWidth(2,36);
	ui->tw_tasks->setColumnWidth(3,50);
	ui->tw_tasks->setColumnWidth(4,40);
	ui->tw_tasks->setColumnWidth(5,36);
	ui->tw_tasks->setColumnWidth(6,36);
	ui->tw_tasks->setColumnWidth(7,40);
	QStringList tasktype_list; //寄存器类型列表
	tasktype_list.append("06");
	tasktype_list.append("10");
	tasktype_list.append("03");
	tasktype_list.append("04");
	for(int i=0;i<task_list.size();i++) //遍历所有任务
	{
		QTableWidgetItem *item;
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 0, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 1, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 2, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 3, item);
		//item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 4, item);
		QComboBox *tmpcombo=new QComboBox(); tmpcombo->addItems(tasktype_list);
		ui->tw_tasks->setCellWidget(i,4,tmpcombo);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 5, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 6, item);
		item = new QTableWidgetItem(); ui->tw_tasks->setItem(i, 7, item);
		item->setFlags(item->flags() & (~(1<<1))); //最后一项不可编辑

		ui->tw_tasks->item(i, 0)->setCheckState(task_list[i].mdbs_buf.enable?Qt::Checked : Qt::Unchecked);
		ui->tw_tasks->item(i, 1)->setText(task_list[i].name.c_str());
		ui->tw_tasks->item(i, 2)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.addr));
		ui->tw_tasks->item(i, 3)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.st));
		((QComboBox *)(ui->tw_tasks->cellWidget(i, 4)))->setCurrentText(QString().sprintf("%02X",task_list[i].mdbs_buf.type));
		ui->tw_tasks->item(i, 5)->setText(QString().sprintf("%d",task_list[i].mdbs_buf.num));
		ui->tw_tasks->item(i, 6)->setText(
			sFormat("%.1f",tick_2_freq(task_list[i].mdbs_buf.freq)).c_str());
		const char *ttab[]={"正常","错误"};
		ui->tw_tasks->item(i, 7)->setText(ttab[task_list[i].mdbs_buf.stat==3?1:0]);
	}
	QObject::connect(ui->tw_tasks,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(on_tw_tasks_itemChanged(QTableWidgetItem*)));
}
void MainWindow::updateData_tasks(void) //从界面更新数据：任务列表
{
	int row=ui->tw_tasks->rowCount();
	int col=ui->tw_tasks->columnCount();
	for(int i=0;i<row;i++) //对于每一行
	{
		bool b;
		task_list[i].mdbs_buf.enable=ui->tw_tasks->item(i, 0)->checkState()==Qt::Checked?1:0;
		task_list[i].name=ui->tw_tasks->item(i, 1)->text().toStdString();
		task_list[i].mdbs_buf.addr=ui->tw_tasks->item(i, 2)->text().toInt();
		task_list[i].mdbs_buf.st=ui->tw_tasks->item(i, 3)->text().toInt();
		task_list[i].mdbs_buf.type=((QComboBox *)(ui->tw_tasks->cellWidget(i, 4)))->currentText().toInt(&b,16);
		task_list[i].mdbs_buf.num=ui->tw_tasks->item(i, 5)->text().toInt();
		float t=ui->tw_tasks->item(i, 6)->text().toFloat();
		task_list[i].mdbs_buf.freq=freq_2_tick(t);
	}
}
void MainWindow::timerEvent(QTimerEvent *event) //100Hz
{
	if(event->timerId() == timerid) //定时器调用，或触发
	{
		task_poll();
		static u32 tick=0;
		if(tick++%30==1)
		{
			updateData_tasks();
			updateData_regs();
		}
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
void MainWindow::on_bt_clear_data_clicked() //清除数据
{
	ui->te_comm_log->clear();
}
////////////////////////////////////////////////////////////////////////////
//					任务部分
////////////////////////////////////////////////////////////////////////////
void MainWindow::on_bt_start_task_clicked() //开始周期任务
{
	if(ui->bt_start_task->text()=="开始周期任务")
	{
		task_start();
		if(is_running==1)
		{
			ui->bt_start_task->setText("结束周期任务");
		}
	}
	else
	{
		task_stop();
		ui->bt_start_task->setText("开始周期任务");
	}
}
void MainWindow::on_bt_add_task_clicked() //添加任务
{
	CMTask tt;
	tt.mdbs_buf.enable=0;
	task_list.push_back(tt);
	updateUI_tasks();
}

void MainWindow::on_bt_del_task_clicked() //删除任务
{
	//删除当前选中的任务
	int cr=ui->tw_tasks->currentRow();
	if(cr>=0)
	{
		task_list.erase(task_list.begin()+cr);
		updateUI_tasks();
	}
}
////////////////////////////////////////////////////////////////////////////
//					寄存器部分
////////////////////////////////////////////////////////////////////////////
void MainWindow::on_bt_add_reg_clicked() //添加寄存器
{
	CMReg tt;
	regs_list.push_back(tt);
	updateUI_regs();
}

void MainWindow::on_bt_del_reg_clicked() //删除寄存器
{
	//删除当前选中的任务
	int cr=ui->tw_regs->currentRow();
	if(cr>=0)
	{
		regs_list.erase(regs_list.begin()+cr);
		updateUI_regs();
	}
}
////////////////////////////////////////////////////////////////////////////
//					帮助
////////////////////////////////////////////////////////////////////////////
void MainWindow::on_bt_help_clicked() //帮助
{
	QMessageBox::about(this,"关于软件","<b>asdf</b>qwer,1234<br/><span style=\"color:red\">poiuj</span>");
}


