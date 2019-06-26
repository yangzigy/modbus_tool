#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include "hexstr.h"

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
	QObject::connect(this,SIGNAL(signal_modbus_lostlock(u8)),this,SLOT(slot_modbus_lostlock(u8)));
	QObject::connect(this,SIGNAL(signal_update_a_reg(u8,u16,u16)),this,SLOT(slot_update_a_reg(u8,u16,u16)));
	QObject::connect(this,SIGNAL(signal_modbus_rxpack(u8*,int)),this,SLOT(slot_modbus_rxpack(u8*,int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}
void MainWindow::ui_initial()
{
	regs_create_UI();
	tasks_create_UI();

	chart0 = new QChart();
	QMargins tmpmarg(0,0,0,0);
	chart0->setMargins(tmpmarg);
	chartView0 = new QChartView(chart0);
	chartView0->setRubberBand(QChartView::RectangleRubberBand);
	ui->gridLayout_8->addWidget(chartView0,0,0);

	timerid=startTimer(10); //初始化定时器

	ui->te_comm_log->setReadOnly(true);
	//测试
//	u8 tb[8]={1,3,0,8,0,2,0xcc,0xba};
//	u8 tb10[]={1,0x10,0,8,0,2,4,0,1,0,2,0xcc,0xba};
//	u8 rb03[]={1,3,2,8,0,2,4,0xcc,0xba};
//	u8 rberr[]={1,3,2,0xcc,0xba};
//	ui->te_comm_log->tx_pack(tb,sizeof(tb));
//	ui->te_comm_log->rx_pack(rb03,sizeof(rb03));
//	ui->te_comm_log->tx_pack(tb10,sizeof(tb10));
//	ui->te_comm_log->rx_lostlock(rb03,sizeof(rb03));
//	ui->te_comm_log->rx_pack(rberr,sizeof(rberr));

	sttime=com_time_getms();
//	QPalette pal = window()->palette();
//	pal.setColor(QPalette::Window, QRgb(0x40434a));//黑灰
//	pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));//白灰
//	window()->setPalette(pal);
	//chartView0->chart()->setTheme(QChart::ChartThemeLight); //普通
	//chartView0->chart()->setTheme(QChart::ChartThemeBlueCerulean); //正常偏暗
//	chartView0->chart()->setTheme(QChart::ChartThemeDark); //暗
	chartView0->chart()->setTheme(QChart::ChartThemeBrownSand); //黄正常
	//chartView0->chart()->setTheme(QChart::ChartThemeBlueNcs); //普通
	//chartView0->chart()->setTheme(QChart::ChartThemeHighContrast); //近似黑白
	//chartView0->chart()->setTheme(QChart::ChartThemeBlueIcy); //普通
	//chartView0->chart()->setTheme(QChart::ChartThemeQt); //普通

	//设置输入限制
	QValidator *validator=new QIntValidator(0,255,this);
	ui->le_addr->setValidator(validator);
	validator=new QIntValidator(1,255,this);
	ui->le_slave_addr->setValidator(validator);
	validator=new QIntValidator(1,123,this);
	ui->le_num->setValidator(validator);
	validator=new QIntValidator(0,65535,this);
	ui->le_reg->setValidator(validator);
	ui->le_06_val->setValidator(validator);
}
void MainWindow::timerEvent(QTimerEvent *event) //100Hz
{
	if(event->timerId() == timerid) //定时器调用，或触发
	{
		task_poll();
		static u32 tick=0;
		if(tick++%30==1)
		{
			regs_update_UI(); //首先看看有没有要更新UI的，然后看是否有UI指令
			regs_update_data(); //将UI数据更新到数据
			tasks_update_UI();
			tasks_update_data(); //将UI数据更新到数据
		}
		if(tick%10==2) //10Hz
		{
			//刷新模式切换
			int ui_is_master=ui->rb_wmod_master->isChecked()?1:0;
			if(is_master==0 && ui_is_master==1) //若slave切换为master
			{
				ui->bt_start_task->setEnabled(true);
				ui->bt_send->setEnabled(true);
				ui->le_slave_addr->setEnabled(true);
			}
			else if(is_master==1 && ui_is_master==0) //若master切换为slave
			{
				ui->bt_start_task->setEnabled(false);
				ui->bt_send->setEnabled(false);
				ui->le_slave_addr->setEnabled(false);
				//重新注册从机的寄存器
				slave_md.address=ui->le_slave_addr->text().toInt();
				//首先回收所有现有的任务结构
				if(slave_md.addr_list)
				{ //要记得删除数据缓存，数据缓存必须new，因为regs_list是动态的
					for(int i=0;i<slave_md.addr_list_n;i++)
					{
						delete slave_md.addr_list[i].buf;
					}
					delete[] slave_md.addr_list;
				}
				slave_md.addr_list=0; slave_md.addr_list_n=0;
				//建立新的结构
				MODBUS_ADDR_LIST *pt=new MODBUS_ADDR_LIST[regs_list.size()];
				for(int i=0;i<regs_list.size();i++)
				{
					pt[i].st=regs_list[i].reg;
					pt[i].num=1;
					pt[i].buf=new u16[1];
					pt[i].addr=regs_list[i].addr;
					pt[i].type=0;
					pt[i].stat=0;
					pt[i].err=0;
					pt[i].buf[0]=regs_list[i].dbuf;
				}
				slave_md.reg(pt,regs_list.size());
			}
			is_master=ui_is_master;
			//刷新从模式的寄存器值
			if(is_master==0) //若是从模式
			{
				MODBUS_ADDR_LIST *pl=slave_md.addr_list;
				for(int k=0;k<slave_md.addr_list_n;k++) //对于所有的从模式地址列表（一般一个地址列表就是一个寄存器）
				{
					for(int i=0;i<pl[k].num;i++) //对于每个地址列表中的寄存器（一般为1）
					{
						u16 reg=pl[k].st+i; //寄存器地址
						for(int j=0;j<regs_list.size();j++) //在所有寄存器列表中找此寄存器
						{
							if(regs_list[j].addr==slave_md.address &&
								regs_list[j].reg==reg)
							{
								pl[k].buf[i]=regs_list[j].dbuf;
								break;
							}
						}
					}
				}
			}
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
		if(is_master) main_md.pack((u8*)buf,n);
		else slave_md.pack((u8*)buf,n);
	}
}
void MainWindow::slot_modbus_lostlock(u8 b) //modbus模块失锁
{
	ui->te_comm_log->rx_lostlock(&b,1); //加入日志
}
void MainWindow::slot_modbus_rxpack(u8 *p,int n) //modbus模块失锁
{
	ui->te_comm_log->rx_pack(p,n); //加入日志
}
void MainWindow::slot_update_a_reg(u8 addr,u16 reg,u16 d) //更新一个寄存器
{
	//首先查找是哪个寄存器
	int i;
	for(i=0;i<regs_list.size();i++)
	{
		if(regs_list[i].addr==addr && regs_list[i].reg==reg)
		{
			regs_list[i].dbuf=d;
			regs_list[i].need_update_UI=1; //加入显示刷新标志
			//加入曲线
			if(regs_list[i].is_curv) //若要显示曲线
			{
				int s_no=regs_list[i].addr*65536+regs_list[i].reg;
				if(curv_map.count(s_no)>0 && curv_map[s_no]) //且曲线列表中有
				{
					u32 tmptime=com_time_getms();
					if(curv_map[s_no]->count()>max_curv_len)
					{
						curv_map[s_no]->removePoints(0,1);
					}
					curv_map[s_no]->append(tmptime-sttime,
							regs_list[i].org_2_val(regs_list[i].dbuf));
					//是否存盘
					if(ui->cb_save_data->isChecked())
					{
						string s=sFormat("%d:%s:%.2f\n",tmptime-sttime,
							curv_map[s_no]->name().toStdString().c_str(),
							regs_list[i].org_2_val(regs_list[i].dbuf));
						extern void rec_log(const char *ps); //记录日志或发出传感数据
						rec_log(s.c_str());
					}
				}
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////
//					数据刷新
////////////////////////////////////////////////////////////////////////////
void MainWindow::regs_update_UI_row(int row) //刷新界面：寄存器
{
	ui->tw_regs->item(row, 0)->setText(regs_list[row].name.c_str());
	ui->tw_regs->item(row, 1)->setText(QString().sprintf("%04X",regs_list[row].dbuf));
	ui->tw_regs->item(row, 2)->setText(QString().sprintf("%f",regs_list[row].org_2_val(regs_list[row].dbuf)));
	ui->tw_regs->item(row, 3)->setText(QString().sprintf("%d",regs_list[row].addr));
	ui->tw_regs->item(row, 4)->setText(QString().sprintf("%d",regs_list[row].reg));
	ui->tw_regs->item(row, 5)->setCheckState(regs_list[row].is_curv?Qt::Checked : Qt::Unchecked);
	ui->tw_regs->item(row, 6)->setText(QString().sprintf("%.2f",regs_list[row].d_k));
	ui->tw_regs->item(row, 7)->setText(QString().sprintf("%.2f",regs_list[row].d_off));
}
void MainWindow::regs_update_UI(void) //刷新界面：寄存器，看标志是否需要刷新
{
	for(int i=0;i<regs_list.size();i++)
	{
		if(regs_list[i].need_update_UI)
		{
			regs_update_UI_row(i);
			regs_list[i].need_update_UI=0;
		}
	}
	//在这里刷新曲线的范围
	if(is_auto_fitscreen)
	{
		auto cs=chart0->series();
		if(cs.size()>0)
		{
			int xmin=0xffffffff,xmax=10000; //时间长度
			float ymin=0,ymax=10; //数据长度
			for(int i=0;i<cs.size();i++) //每一条线
			{
				QLineSeries *ps=(QLineSeries *)cs.at(i);
				for(int j=0;j<ps->count();j++)
				{
					QPointF qf=ps->at(j);
					if(xmin>qf.rx()) xmin=qf.rx();
					if(xmax<qf.rx()) xmax=qf.rx();
					if(ymin>qf.ry()) ymin=qf.ry();
					if(ymax<qf.ry()) ymax=qf.ry();
				}
			}
			chart0->axisX()->setRange(xmin, xmax);
			chart0->axisY()->setRange(ymin,ymax);
		}
	}
	if(ui->cb_auto_fitscreen->isChecked()) is_auto_fitscreen=1;
	else is_auto_fitscreen=0;
}
void MainWindow::regs_create_UI(void) //从数据更新界面：寄存器列表
{
	ui->tw_regs->clear();
	ui->tw_regs->setColumnCount(8);
	ui->tw_regs->setRowCount(regs_list.size());
	ui->tw_regs->setHorizontalHeaderLabels(QStringList() <<
		"名称"<<"原始值"<<"值"<<"地址"<<"寄存器"<<"曲线"<<"系数"<<"偏移");
	ui->tw_regs->setColumnWidth(0,60);
	ui->tw_regs->setColumnWidth(1,50);
	ui->tw_regs->setColumnWidth(2,70);
	ui->tw_regs->setColumnWidth(3,40);
	ui->tw_regs->setColumnWidth(4,50);
	ui->tw_regs->setColumnWidth(5,35);
	ui->tw_regs->setColumnWidth(6,50);
	ui->tw_regs->setColumnWidth(7,50);

	for(int i=0;i<regs_list.size();i++) //遍历所有的寄存器
	{
		QTableWidgetItem *item;
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 0, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 1, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 2, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 3, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 4, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 5, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 6, item);
		item = new QTableWidgetItem(); ui->tw_regs->setItem(i, 7, item);
		regs_update_UI_row(i);
	}
}
void MainWindow::regs_update_data(void) //从界面更新数据：寄存器列表
{
	int row=ui->tw_regs->rowCount();
	for(int i=0;i<row;i++) //对于每一行
	{
		bool b;
		regs_list[i].name=ui->tw_regs->item(i,0)->text().toStdString();
//1、若修改了系数，需要更新值
		float fd_k=ui->tw_regs->item(i, 6)->text().toFloat();
		float fd_off=ui->tw_regs->item(i, 7)->text().toFloat();
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
//2、若修改值，需要更新值
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
//3、若修改了寄存器
		regs_list[i].addr=ui->tw_regs->item(i,3)->text().toInt();
		regs_list[i].reg=ui->tw_regs->item(i, 4)->text().toInt();
//4、曲线
		regs_list[i].is_curv=ui->tw_regs->item(i, 5)->checkState()?1:0;
		int s_no=regs_list[i].addr*65536+regs_list[i].reg;
		if(regs_list[i].is_curv) //若要显示曲线
		{
			if(curv_map.count(s_no)<=0) //且曲线列表中没有
			{
				QLineSeries *tmpseries=new QLineSeries(chart0);
				tmpseries->setName(regs_list[i].name.c_str());
				tmpseries->setUseOpenGL(true); //使用OpenGL加速显示
				chart0->addSeries(tmpseries);
				curv_map[s_no]=tmpseries;
				chart0->createDefaultAxes();
			}
		}
		else if(curv_map.count(s_no)>0) //若不显示曲线,且曲线列表中有
		{
			if(curv_map[s_no])
			{
				chart0->removeSeries(curv_map[s_no]);
				delete curv_map[s_no];
			}
			curv_map.erase(s_no);
		}
	}
	//反向查找曲线
	for(auto &it:curv_map)
	{
		int i;
		for(i=0;i<regs_list.size();i++)
		{
			int s_no=regs_list[i].addr*65536+regs_list[i].reg;
			if(s_no==it.first && regs_list[i].is_curv) break;//若找到了
		}
		if(i==regs_list.size()) //若没找到
		{ //删除此曲线
			if(it.second)
			{
				chart0->removeSeries(it.second);
				delete it.second;
			}
			curv_map.erase(it.first);
			break; //一次只删一个
		}
	}
}
/////////////////////////////////////////////////////////////////////////
string get_task_stat_str(MODBUS_ADDR_LIST &task)
{
	const char *ttab[]={"待执行","发送中","正确","超时","错误"};
	u8 stat=task.stat;
	string s="";
	if(stat>=4) s=sFormat("err:%02X",task.err);
	else s=ttab[stat];
	return s;
}
void MainWindow::tasks_update_UI_row(int row) //刷新界面：任务
{
	ui->tw_tasks->item(row, 0)->setCheckState(task_list[row].enable?Qt::Checked : Qt::Unchecked);
	ui->tw_tasks->item(row, 1)->setText(task_list[row].name.c_str());
	ui->tw_tasks->item(row, 2)->setText(QString().sprintf("%d",task_list[row].mdbs_buf.addr));
	ui->tw_tasks->item(row, 3)->setText(QString().sprintf("%d",task_list[row].mdbs_buf.st));
	((QComboBox *)(ui->tw_tasks->cellWidget(row, 4)))->setCurrentText(QString().sprintf("%02X",task_list[row].mdbs_buf.type));
	ui->tw_tasks->item(row, 5)->setText(QString().sprintf("%d",task_list[row].mdbs_buf.num));
	ui->tw_tasks->item(row, 6)->setText(
			sFormat("%.1f",task_list[row].freq).c_str());
	string s=get_task_stat_str(task_list[row].mdbs_buf);
	ui->tw_tasks->item(row, 7)->setText(s.c_str());
}
void MainWindow::tasks_update_UI(void) //刷新界面：任务
{
	//任务界面基本只接收指令，数据更新只有状态
	for(int i=0;i<task_list.size();i++) //遍历所有任务
	{
		string s=get_task_stat_str(task_list[i].mdbs_buf);
		ui->tw_tasks->item(i, 7)->setText(s.c_str());
	}
}
void MainWindow::tasks_create_UI(void) //从数据更新界面：任务列表
{
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
	ui->tw_tasks->setColumnWidth(7,50);
	QStringList tasktype_list; //寄存器类型列表
	//tasktype_list.append("06");
	//tasktype_list.append("10");
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

		tasks_update_UI_row(i);
	}
}
void MainWindow::tasks_update_data(void) //从界面更新数据：任务列表
{
	int row=ui->tw_tasks->rowCount();
	for(int i=0;i<row;i++) //对于每一行
	{
		bool b;
		task_list[i].enable=ui->tw_tasks->item(i, 0)->checkState()==Qt::Checked?1:0;
		task_list[i].name=ui->tw_tasks->item(i, 1)->text().toStdString();
		task_list[i].mdbs_buf.addr=ui->tw_tasks->item(i, 2)->text().toInt();
		task_list[i].mdbs_buf.st=ui->tw_tasks->item(i, 3)->text().toInt();
		task_list[i].mdbs_buf.type=((QComboBox *)(ui->tw_tasks->cellWidget(i, 4)))->currentText().toInt(&b,16);
		task_list[i].mdbs_buf.num=ui->tw_tasks->item(i, 5)->text().toInt();
		float t=ui->tw_tasks->item(i, 6)->text().toFloat();
		task_list[i].freq=t;
	}
}

/////////////////////////////////////////////////////////////////////////
//					界面响应
////////////////////////////////////////////////////////////////////////////
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
	curv_map.clear();
	chart0->removeAllSeries(); //去掉所有曲线
	sttime=com_time_getms();
}
void MainWindow::on_bt_fitscreen_clicked() //适应屏幕
{
	is_auto_fitscreen=2;
}
////////////////////////////////////////////////////////////////////////////
//					任务部分
void MainWindow::on_bt_start_task_clicked() //开始周期任务
{
	if(ui->bt_start_task->text()=="开始周期任务")
	{
		task_start();
		if(is_running==1)
		{
			ui->bt_start_task->setText("结束周期任务");
			ui->bt_send->setEnabled(false);
			ui->bt_import_cfg->setEnabled(false);
			ui->bt_add_task->setEnabled(false);
			ui->bt_del_task->setEnabled(false);
			ui->rb_wmod_master->setEnabled(false);
			ui->rb_wmod_slave->setEnabled(false);
		}
	}
	else
	{
		task_stop();
		ui->bt_start_task->setText("开始周期任务");
		ui->bt_send->setEnabled(true);
		ui->bt_import_cfg->setEnabled(true);
		ui->bt_add_task->setEnabled(true);
		ui->bt_del_task->setEnabled(true);
		ui->rb_wmod_master->setEnabled(true);
		ui->rb_wmod_slave->setEnabled(true);
	}
}
void MainWindow::on_bt_add_task_clicked() //添加任务
{
	CMTask tt; //
	task_list.push_back(tt);
	tasks_create_UI();
}

void MainWindow::on_bt_del_task_clicked() //删除任务
{
	//删除当前选中的任务
	int cr=ui->tw_tasks->currentRow();
	if(cr>=0)
	{
		task_list.erase(task_list.begin()+cr);
		tasks_create_UI();
	}
}
void MainWindow::on_bt_overtime_10ms_clicked() //设置超时时间
{
	bool b=false;
	int ot=ui->le_overtime_10ms->text().toInt(&b);
	if(b)
	{
		main_md.timeout=ot;
	}
	else
	{
		QMessageBox::warning(this,"输入错误","超时时间单位10ms，整数");
	}
}
CMTask single_task; //单次发送任务
u8 single_task_buf[256];
void MainWindow::on_bt_send_clicked() //单次发送
{
	if(is_running!=0) //只能在结束状态下使用
	{
		return ;
	}
	//取得此任务的参数
	single_task.need_update_UI=1;

	single_task.mdbs_buf.type=3; //类型
	if(ui->rb_04->isChecked()) single_task.mdbs_buf.type=4;
	if(ui->rb_06->isChecked()) single_task.mdbs_buf.type=6;
	if(ui->rb_10->isChecked()) single_task.mdbs_buf.type=0x10;

	single_task.mdbs_buf.addr=ui->le_addr->text().toInt(); //地址
	single_task.mdbs_buf.st=ui->le_reg->text().toInt(); //寄存器
	if(single_task.mdbs_buf.type==6) //数量
	{
		single_task.mdbs_buf.num=1;
	}
	else
	{
		int num=ui->le_num->text().toInt();
		MINMAX(num,1,MAX_REG_NUM);
		single_task.mdbs_buf.num=num;
	}
	single_task.mdbs_buf.buf=(u16*)single_task_buf;
	if(single_task.mdbs_buf.type==6) //若写单
	{
		bool b;
		single_task.mdbs_buf.buf[0]=ui->le_06_val->text().toInt(&b,16);
	}
	else if(single_task.mdbs_buf.type==0x10) //若写多
	{
		vector<u8> v;
		string s=ui->le_10_data->text().toStdString();
		int r=str2bin(s.c_str(),s.size(),v);
		if(r==0 && v.size()==single_task.mdbs_buf.num*2)
		{
			for(int i=0;i<v.size();i+=2) //换端
			{
				u8 t=v[i];
				v[i]=v[i+1];
				v[i+1]=t;
			}
			memcpy(single_task.mdbs_buf.buf,&(v[0]),v.size());
		}
		else
		{//错误
			QMessageBox::information(this,"错误","数据数量错误");
			return ;
		}
	}
	single_task.enable=1;

	//注册，开始
	main_md.add_task(&(single_task.mdbs_buf)); //主机注册任务
}
////////////////////////////////////////////////////////////////////////////
//					寄存器部分
void MainWindow::on_bt_add_reg_clicked() //添加寄存器
{
	CMReg tt;
	regs_list.push_back(tt);
	regs_create_UI();
}
void MainWindow::on_bt_del_reg_clicked() //删除寄存器
{
	//删除当前选中的任务
	int cr=ui->tw_regs->currentRow();
	if(cr>=0)
	{
		regs_list.erase(regs_list.begin()+cr);
		regs_create_UI();
	}
}
////////////////////////////////////////////////////////////////////////////
//					配置与帮助
////////////////////////////////////////////////////////////////////////////
void MainWindow::on_bt_help_clicked() //帮助
{
	QFile nFile(":/help.html");
	if(!nFile.open(QFile::ReadOnly))
	{
		qDebug() << "could not open file for reading";
		return;
	}
	string nText =nFile.readAll().data();
	nText=com_replace(nText,"src=\"","src=\"qrc:/");
	//widget_help->setGeometry(100,100,400,500);
	widget_help->setStyleSheet("font-size:14px;");
	widget_help->setMinimumWidth(800);
	widget_help->setMinimumHeight(500);
	widget_help->setHtml(nText.c_str());
	widget_help->show();
}
void MainWindow::clean_cfg(void) //清理配置
{
	regs_list.clear();
	ui->tw_regs->clear();
	task_list.clear();
	ui->tw_tasks->clear();
	curv_map.clear();
	chart0->removeAllSeries(); //去掉所有曲线
	sttime=com_time_getms();
}
void MainWindow::on_bt_import_cfg_clicked() //导入配置
{
	auto name=QFileDialog::getOpenFileName(0,"","","txt文件(*.txt)");
	if(name!="")
	{
		string text=read_textfile(name.toStdString().c_str());
		Json::Reader reader;
		reader.parse(text.c_str(),config,false); //可以有注释,false不会复制
		pjson(config); //输出配置字符

		clean_cfg(); //首先清理配置
		app_ini(config);
		regs_create_UI();
		tasks_create_UI();
	}
}
void MainWindow::on_bt_save_cfg_clicked() //保存配置
{
	auto name=QFileDialog::getSaveFileName (0,"","","txt文件(*.txt)");
	if(name!="")
	{
		Json::Value v;
		for(int i=0;i<regs_list.size();i++)
		{
			v["datalist"][i]=regs_list[i].toJson();
		}
		for(int i=0;i<task_list.size();i++)
		{
			v["tasklist"][i]=task_list[i].toJson();
		}
		Json::StyledWriter writer;
		string s=writer.write(v);
		CComFile cf;
		cf.open(name.toStdString().c_str(),"wb");
		cf.write((u8*)s.c_str(),s.size());
	}
}
