#ifndef MQCHART_H
#define MQCHART_H

#include "json.h"
#include "common.h"
#include <QDialog>
#include <QtCharts>
#include <QLabel>

class MQChartView : public QChartView
{
public:
	MQChartView(QWidget *parent = nullptr):QChartView()
	{
		chart0 = new QChart();
		QMargins tmpmarg(0,0,0,0);
		chart0->setMargins(tmpmarg);

		m_coord=new QGraphicsSimpleTextItem(chart0);
		QFont font;
		font.setPixelSize(16);
		m_coord->setFont(font);

		m_line=new QGraphicsLineItem(chart0);
		QPen pen(Qt::black,1,Qt::DashDotLine);
		m_line->setPen(pen);
		m_line->setLine(0,0,0,100);

		setChart(chart0);
		setRubberBand(QChartView::RectangleRubberBand);
	}
	string s_mouse; //鼠标字符
	string s_val; //曲线值字符
	QChart *chart0;
	float rx_min=0; //获取当前坐标范围
	float ry_min=0;
	float rx_max=0;
	float ry_max=0;
	void get_chart_minmax(void)
	{
		rx_min=static_cast<QValueAxis *>(chart0->axisX())->min();
		ry_min=static_cast<QValueAxis *>(chart0->axisY())->min();
		rx_max=static_cast<QValueAxis *>(chart0->axisX())->max();
		ry_max=static_cast<QValueAxis *>(chart0->axisY())->max();
	}
	void mousePressEvent(QMouseEvent * event)
	{
		qreal x = (event->pos()).x();
		qreal y = (event->pos()).y();
		double xVal = chart0->mapToValue(event->pos()).x();
		double yVal = chart0->mapToValue(event->pos()).y();

		//查找各曲线的值
		s_mouse=sFormat("鼠标:x:%.3f,y:%1.0f | ",xVal,yVal);
		s_val="";
		float pre_x=0;
		float pre_y=0;
		auto ser=chart0->series();
		for(int i=0;i<ser.size();i++)
		{
			QLineSeries* ls=(QLineSeries*)ser[i];
			s_val+=ls->name().toStdString()+":";
			auto ps=ls->pointsVector();
			for(int j=0;j<ps.size();j++)
			{
				auto &tp=ps[j];
				float tx=tp.x();
				float ty=tp.y();
				if(pre_x<xVal && xVal<=tx) 
				{
					float o0=xVal-pre_x;
					float o1=tx-xVal;
					float out=o1*pre_y + o0*ty;
					out/=(o0+o1);
					s_val+=sFormat("%1.0f ",out);
					break;
				}
				pre_x=tx;
				pre_y=ty;
			}
		}
		m_coord->setPos(5, 5);
		m_coord->setText((s_mouse+s_val).c_str());
		m_line->setLine(x,0,x,height());
		if(event->buttons() & Qt::RightButton) //右键不给系统处理了，放大改用滚轮
		{
			is_rb_pressed=true;
			return ;
		}
		is_rb_pressed=false;
		QChartView::mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent * event)
	{
		if(is_rb_pressed) //右键不给系统处理了，放大改用滚轮
		{
			return ;
		}
		QChartView::mouseReleaseEvent(event);
	}
	void mouseMoveEvent(QMouseEvent * event)
	{
		int x = (event->pos()).x();
		int y = (event->pos()).y();
		float xVal = chart0->mapToValue(event->pos()).x();
		float yVal = chart0->mapToValue(event->pos()).y();

		s_mouse=sFormat("鼠标:x:%.3f,y:%1.0f | ",xVal,yVal);
		m_coord->setText((s_mouse+s_val).c_str());

		if(event->buttons() & Qt::RightButton) //右键拖动
		{
			QPointF p1=chart0->mapToValue(QPointF(mouse_point.x(),mouse_point.y()));
			float rdx=xVal-p1.x();
			float rdy=yVal-p1.y(); //物坐标的增量
			get_chart_minmax(); //获取曲线坐标的极值
			chart0->axisX()->setRange(rx_min-rdx, rx_max-rdx);
			chart0->axisY()->setRange(ry_min-rdy, ry_max-rdy);
			mouse_point.setX(event->x());
			mouse_point.setY(event->y());
			return ;
		}
		mouse_point.setX(event->x());
		mouse_point.setY(event->y());
		QChartView::mouseMoveEvent(event);
	}
	void wheelEvent(QWheelEvent *event)
	{
		float t=event->delta()/1000.0f; //比例
		t=com_limit(t,-0.5f,0.5f);
		get_chart_minmax(); //获取曲线坐标的极值
		float lx=rx_max-rx_min;
		float ly=ry_max-ry_min; //长度
		t=1-t;
		float rdx=(lx*t-lx)/2;
		float rdy=(ly*t-ly)/2;
		chart0->axisX()->setRange(rx_min-rdx, rx_max+rdx); //减正为放大，减负为缩小
		chart0->axisY()->setRange(ry_min-rdy, ry_max+rdy);
	}
private:
	QVector2D mouse_point; //鼠标点
	QGraphicsSimpleTextItem *m_coord; //曲线值文字
	QGraphicsLineItem *m_line; //竖线
	bool is_rb_pressed=false;
};

#endif // 

