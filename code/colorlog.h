#ifndef COLORLOG_H
#define COLORLOG_H

#include "json.h"
#include "common.h"
#include <QString>
#include "qlabel.h"
#include <QLayout>
#include <QTextEdit>
#include "hexstr.h"

class CColorLog : public QTextEdit//显示块类
{
public:
	~CColorLog()
	{
	}
	QTextCursor tcursor;
	CColorLog(QWidget *p= 0):QTextEdit(p)
	{
		tcursor=this->textCursor();
		tcursor.movePosition(QTextCursor::End);
		setTextCursor(tcursor);
		//insertHtml("<span style=\"color:red\">asdf</span>");
		//insertHtml("<span style=\"color:green;background-color:blue\">This is some text!</span>");
	}
	int stat=0; //0:tx,1:rx,2:lost
	string str_buf; //字符缓冲，避免操作qt对象，qt对象会动
	void check_len(void) //增加的长度
	{
		if(str_buf.size()>KB(30)) //若超过限制，则进行限制
		{
			int p=str_buf.find("<br/>", str_buf.size()/3);
			printf("p:%d,len:%d\n",p,str_buf.size());
			if(p<=0) return ;
			str_buf=str_buf.substr(p,str_buf.size()-p);
			setHtml(str_buf.c_str());
		}
		QScrollBar *scrollbar = verticalScrollBar();
		if(scrollbar)
		{
			scrollbar->setSliderPosition(scrollbar->maximum());
		}
	}
	void tx_pack(u8 *p,int n) //发送数据
	{
		if(n<8) return ;
		setTextCursor(tcursor);
		char sbuf[25600]={0};
		char *pbuf=sbuf;
		pbuf+=sprintf(pbuf,"<br/><span style=\"color:red;font-weight:800\">%02X</span> ",p[0]);
		pbuf+=sprintf(pbuf,"<span style=\"color:yellow;font-weight:800\">%02X</span> ",p[1]);
		pbuf+=sprintf(pbuf,"<span style=\"color:blue;font-weight:800\">%02X %02X</span> ",p[2],p[3]);
		pbuf+=sprintf(pbuf,"<span style=\"color:Fuchsia;font-weight:800\">%02X %02X</span> ",p[4],p[5]);
		int pp=6;
		if(n>8) //若是10包
		{
			pbuf+=sprintf(pbuf," %02X<span style=\"color:green;font-weight:800\">",p[pp++]);
			for(;pp<n-2;pp++)
			{
				pbuf+=sprintf(pbuf," %02X",p[pp]);
			}
			pbuf+=sprintf(pbuf,"</span> ");
		}
		//CRC
		pbuf+=sprintf(pbuf,"%02X %02X",p[pp],p[pp+1]);
		str_buf+=sbuf;
		setHtml(str_buf.c_str());
		//insertHtml(sbuf);
		check_len();
		stat=0;
	}
	void rx_lostlock(u8 *p,int n) //接收失锁数据
	{
		setTextCursor(tcursor);
		char sbuf[25600]={0};
		char *pbuf=sbuf;
		static int lllen=0;
		if(stat!=2)
		{
			pbuf+=sprintf(pbuf,"<br/>");
		}
		for(int i=0;i<n;i++)
		{
			pbuf+=sprintf(pbuf,"<span style=\"background-color:white\">%02X </span>",p[i]);
			if(lllen++>=20)
			{
				pbuf+=sprintf(pbuf,"<br/>");
				lllen=0;
			}
		}
		str_buf+=sbuf;
		setHtml(str_buf.c_str());
		check_len();
		stat=2;
	}
	void rx_pack(u8 *p,int n) //接收数据
	{
		if(n<5) return ;
		setTextCursor(tcursor);
		char sbuf[25600]={0};
		char *pbuf=sbuf;
		pbuf+=sprintf(pbuf,"<br/><span style=\"background-color:red\">%02X</span> ",p[0]);
		pbuf+=sprintf(pbuf,"<span style=\"background-color:yellow\">%02X</span> ",p[1]);
		int pp=2;
		if(n==5) //若是错误包
		{
			pbuf+=sprintf(pbuf,"<span style=\"background-color:red\">%02X</span> ",p[pp++]);
		}
		else if(n==8) //若是06/10包
		{
			pbuf+=sprintf(pbuf,"<span style=\"background-color:blue\">%02X %02X</span> ",p[2],p[3]);
			pbuf+=sprintf(pbuf,"<span style=\"background-color:Fuchsia\">%02X %02X</span> ",p[4],p[5]);
			pp=6;
		}
		else //读数据包
		{
			pbuf+=sprintf(pbuf," %02X<span style=\"background-color:green\">",p[pp++]);
			for(;pp<n-2;pp++)
			{
				pbuf+=sprintf(pbuf," %02X",p[pp]);
			}
			pbuf+=sprintf(pbuf,"</span> ");
		}
		//CRC
		pbuf+=sprintf(pbuf,"%02X %02X",p[pp],p[pp+1]);
		str_buf+=sbuf;
		setHtml(str_buf.c_str());
		check_len();
		stat=1;
	}
	void tx_slave_pack(u8 *p,int n) //从机发送数据
	{
		if(n<5) return ;
		setTextCursor(tcursor);
		char sbuf[25600]={0};
		char *pbuf=sbuf;
		pbuf+=sprintf(pbuf,"<br/><span style=\"font-weight:800;color:red\">%02X</span> ",p[0]);
		pbuf+=sprintf(pbuf,"<span style=\"font-weight:800;color:yellow\">%02X</span> ",p[1]);
		int pp=2;
		if(n==5) //若是错误包
		{
			pbuf+=sprintf(pbuf,"<span style=\"font-weight:800;color:red\">%02X</span> ",p[pp++]);
		}
		else if(n==8) //若是06/10包
		{
			pbuf+=sprintf(pbuf,"<span style=\"font-weight:800;color:blue\">%02X %02X</span> ",p[2],p[3]);
			pbuf+=sprintf(pbuf,"<span style=\"font-weight:800;color:Fuchsia\">%02X %02X</span> ",p[4],p[5]);
			pp=6;
		}
		else //读数据包
		{
			pbuf+=sprintf(pbuf," %02X<span style=\"font-weight:800;color:green\">",p[pp++]);
			for(;pp<n-2;pp++)
			{
				pbuf+=sprintf(pbuf," %02X",p[pp]);
			}
			pbuf+=sprintf(pbuf,"</span> ");
		}
		//CRC
		pbuf+=sprintf(pbuf,"%02X %02X",p[pp],p[pp+1]);
		str_buf+=sbuf;
		setHtml(str_buf.c_str());
		check_len();
		stat=1;
	}
};

#endif // 

