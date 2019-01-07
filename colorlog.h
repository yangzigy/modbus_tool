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
		//textCursor().movePosition(QTextCursor::EndOfBlock);
		tcursor=this->textCursor();
		tcursor.movePosition(QTextCursor::EndOfBlock);
		setTextCursor(tcursor);
		//insertHtml("<span style=\"color:red\">asdf</span>");
		//insertHtml("<span style=\"color:green;background-color:blue\">This is some text!</span>");
	}
	int stat=0; //0:tx,1:rx,2:lost
	u32 total_len=0; //总字符数
	void check_len(u32 addlen) //增加的长度
	{
		total_len+=addlen;
		if(total_len>MB(10)) //若超过限制，则进行限制
		{
			QString	ht=toHtml();
			cout<<"html size "<<ht.size()<<endl;
			int p=ht.indexOf("<br/>",ht.size()*2/3);
			ht.remove(0,p);
			setHtml(ht);
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
		insertHtml(sbuf);
		check_len(pbuf-sbuf);
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
		insertHtml(sbuf);
		check_len(pbuf-sbuf);
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
		insertHtml(sbuf);
		check_len(pbuf-sbuf);
		stat=1;
	}
};

#endif // 

