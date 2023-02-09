/*
文件名：common.h
创建时间：2014-4-16
版本：	V1.1			2019-01-15
版本：	V1.2			2019-08-21
功能：
	C++常用功能夸平台库，32位、64位的windows与linux
	1、时间功能
	2、字符扩展
	3、安全跨平台文件访问
	4、调试与日志
	5、数值工具
	6、事件驱动
	7、python扩展
*/
#ifndef COMMON_H
#define COMMON_H

#include "main.h"

/////////////////////////////////////////////////////////////////////////////////
//跨平台定义
#if (!defined(WIN32) && !defined(WIN64))
	#define PATH_CHAR	'/'
	#define PATH_CHAR_OTHER	'\\'
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#define close_sock	close
#else
	#define PATH_CHAR	'\\'
	#define PATH_CHAR_OTHER	'/'
	#include <WinSock2.h>
	#include <windows.h>
	#undef UNICODE
	#pragma comment(lib,"ws2_32.lib")
	#define close_sock	closesocket
	typedef int socklen_t;
#endif

#ifndef __GNUC__

#define com_fseek	_fseeki64//32位与64位通用
#define com_ftell	_ftelli64//32位与64位通用

int strncasecmp(const char *s1, const char *s2,int n);
int strcasecmp(const char *s1, const char *s2);

#else

#ifdef __i386__
#define com_fseek	fseek
#define com_ftell	ftell
#else
#define com_fseek	fseeko64
#define com_ftell	ftello64
#endif

#endif

/////////////////////////////////////////////////////////////////////////////////
//1、时间功能
void delay(int ms);
u32 com_time_getms(void);//通用的获得当前ms计数的函数

class CDateTime//时间为系统时间
{
public:
	CDateTime(void)
	{format_str="%Y-%m-%d %H:%M:%S";utc=0;st_time=0;real=0;area_tick=0;}
	CDateTime(const char *p)//设置起始时间的构造函数
	{format_str="%Y-%m-%d %H:%M:%S";utc=0;st_time=0;real=0;set_st(p);area_tick=0;}
	~CDateTime(void){}
	const char *format_str;//格式化字符串

	time_t utc;//临时的秒脉冲(相对)
	time_t st_time;//起始累计时间
	time_t real;//临时秒计数（绝对），相对于系统的起始时间点(目标时区)
	//time_t area_tick;//夸时区的秒数
	int area_tick;//夸时区的秒数

	tm *datetime;//临时年月日
	void set_st(const char *p)//设置起始时间
	{
		st_time=parse(p,format_str);
		utc=0;
	}
	string ToString(const char *p)
	{
		real=st_time+utc + area_tick;
		datetime=localtime(&real);
		char buf[64]={0};
		strftime(buf,sizeof(buf),p,datetime);
		return buf;
	}
	string ToString(void){return ToString(format_str);}
	string Now(const char *p){utc=time(0)-st_time;return ToString(p);}
	string Now(void){return Now(format_str);}
	void update(void){datetime=localtime(&real);}
	void update(time_t d){utc=d;real=utc+st_time + area_tick;update();}
	string utc2str(time_t d,const char *p)
	{
		update(d);
		return ToString(p);
	}
	string utc2str(time_t d){return utc2str(d,format_str);}
	//输入本地时间，输出目标时区的时间
	time_t parse(const char *p,const char *des)
	{
		char *strptime(const char *s, const char *format, struct tm *tm);
		strptime(p,des,&tm_);
		tm_.tm_isdst=-1;
		datetime=&tm_;
		real=mktime(&tm_) - area_tick;
		utc=real-st_time;
		return utc;
	}
private:
	tm tm_;
};

class CSamTime	//测量程序时间的类
{
public:
	CSamTime(void){dt=0;k=0;last_t=0;}
	~CSamTime(void){}
	float k;	//低通滤波的系数，为0~1
	float dt;
	u32 time1;
	u32 time2;
	void start(void)	//开始计时
	{
		time1=com_time_getms();
	}
	float stop(void)	//停止计时
	{
		time2=com_time_getms();
		dt=dt*k + (time2-time1)*(1-k);
		return dt;
	}
	//对一系列点计时
	vector<float> stlist;	//时间差结果列表
	u32 sam_n;	//当前采样的位置
	u32 last_t;
	void sample_ini(int n)	//输入要记多少个值
	{
		sam_n=0;
		stlist.resize(n);
	}
	void sample1(void)	//开始序列计时
	{
		sam_n=1;
		last_t=com_time_getms();
	}
	void sample(void)	//序列中计时
	{
		if (sam_n>stlist.size() || sam_n==0)
		{
			return ;//不工作
		}
		u32 cur_t=com_time_getms();
		//计算当前位置的平均数
		stlist[sam_n-1]=stlist[sam_n-1]*k + (cur_t-last_t)*(1-k);
		last_t=cur_t;
		sam_n++;
	}
	float delta_t(void)//测量任意时间间隔
	{
		u32 tmp=com_time_getms();
		if (last_t==0)
		{
			last_t=tmp;
			return 0;
		}
		dt=dt*k + (tmp-last_t)*(1-k);
		last_t=tmp;
		return dt;
	}
};

/////////////////////////////////////////////////////////////////////////////////
//2、字符扩展
void com_strLower(string &s);
void com_strLower(char *s);
string com_trim(string &s);//去除首尾空格
string com_replace(string str,const char *target,const char *src);
string com_replace(string &str,char target,char src);
string sFormat(const char *format,va_list args);
string sFormat(const char *format,...);//格式化字符串
vector<string> com_split(string &s,const char *c);
vector<char*> com_split(char *p,const char *spl);
string operator + (const char *a,string &b); //使字符串能够与string相加

class CFilePath
{
public:
	CFilePath(void){}
	~CFilePath(void){}

	string path;//路径"C:\"
	string name_ext;//文件名"a.txt"
	string ext;//扩展名".txt"
	string name;//基本名"a"
	string path_name_ext;//全名"C:\a.txt"
	string path_name;//路径基本名"C:\a"
	// 通过文件名设置
	bool setName(const char *s);
	bool setName(string s);
	// 通过路径设置，需要检查路径有效性
	bool setPath(const char *s);
	bool setPath(string s);

	void operator =(const char * s);
	void operator =(string s);
};

/////////////////////////////////////////////////////////////////////////////////
//3、安全跨平台文件访问
s64 get_file_size(FILE* fp);//获取文件长度
FILE *com_fopen(const char *s,const char *flag);
//调用shell
void print_error(const char *name);//输出错误信息
#if (!defined(WIN32) && !defined(WIN64))
string com_popen(const char *scmd);//打开只读管道获取命令输出
int _system(const char *command); //不复制内存的调用方式
#else
#define _system system
#endif

class CComFile
{
public:
	CComFile(void){f=0;len=0;}
	~CComFile(void){close();}

	CFilePath filename;
	FILE *f;
	s64 len;
	int open(string &name,const char *mod);
	int open(const char *name,const char *mod);
	int close();
	int seek(u64 off);//从起始开始
	s64 file_len(void);//也更新len
	s64 read(void *p,u64 n);//返回实际读取的字节数
	s64 read_safe(void *p,u64 n);//带报错的读操作
	s64 write(const void *p,u64 n);//返回实际写入的字节数
};
//离线处理函数,将一个文件以缓冲的方式分批读入
//并调用回调函数进行处理。回调函数中指名当前调用者的数据
void offline_pro(CComFile &file,u64 st,u64 end,u64 bufn,
			int (*fun)(u8 *p,u64 n,u64 offset,void *th),void *obj);

//读取文本文件
string read_textfile(const char *filename);
void list_dir(const char *path,const char *ext,vector<string> &rst); //输入路径名带/，输扩展名带. 出文件名列表，不含全路径
int mkdir_1(const char *dirname); //建立1级目录
int mkdir_p(const char *dirname); //建立多级目录  案例//mkdir_p("./c/b/");
/////////////////////////////////////////////////////////////////////////////////
//4、调试与日志
extern string exepath;//本可执行文件的路径
void start_program(void);

class CLogFile
{
public:
	CComFile flog;
	CDateTime date; //不可修改时间格式
	string fname_time_fmt="%Y%m%d_%H%M%S"; //用于文件名的时间格式
	string ts_fmt="%H%M%S "; //用于时间戳的时间格式，空则为不写时间戳
	bool is_stdout=true; //是否在stdout打印
	bool is_logfile=true; //是否在日志文件输出
	CFilePath fp; //记录文件基础路径
	string prefix=""; //前缀
	string suffix=".txt"; //后缀
	u32 restart_T=3600; //重新建立文件的周期(s)

	u32 pre_utc=0; //上次记录的时间
	string filename = ""; //记录文件的全路径（只读）
	CLogFile()
	{
		date.format_str="%Y%m%d_%H%M%S";
		fp="./";
	}
	virtual void create(string &fname) //建立文件
	{//可重写此函数，实现文件夹建立、文件头
		//QDir().mkpath(fp.path.c_str());
		flog.open(fname.c_str(),"ab"); //自动关闭之前的
		pre_utc=time(0);
	}
	virtual void close(void)
	{
		flog.close();
	}
	virtual void update_file_name(void) //刷新文件名
	{
		if(!is_logfile) return ; //若不记录日志
		u32 t=time(0);
		u32 t_h=(t/restart_T)*restart_T; //整时间点
		if(flog.f==0 || pre_utc<t_h) //无文件或到了整数时间切换文件
		{
			string fname=fp.path+prefix+date.Now(fname_time_fmt.c_str())+suffix;
			this->create(fname);
		}
	}
	//////////////////////////////////////////////////////////////////////
	//	底层写入，可写二进制或文本
	virtual void log_pass(const void *p,int n)//直接写日志,二进制
	{
		if(flog.f==0) return ;
		flog.write(p,n);
		fflush(flog.f); //以文本形式记录，执行刷新
	}
	virtual void log_pass(string &s)//直接写日志，文本
	{
		if(is_stdout) cout<<s; //这里是文本单独的处理，考虑输出到stdout
		log_pass(s.c_str(),s.size()); //二进制和文本都可以
	}
	void log_pass(const char *p) { string s=p; log_pass(s); }
	//////////////////////////////////////////////////////////////////////
	//	带有文件名更新的写入，可写二进制或文本
	virtual void write(const void *p,int n) //写二进制日志，刷新文件名
	{
		update_file_name();
		log_pass(p,n);
	}
	virtual CLogFile &write(string &s) //写文本，刷新文件名，刷新缓存
	{
		update_file_name();
		log_pass(s);
		return *this;
	}
	CLogFile & write(const char *ps) { string s=ps; return write(s); }
	//////////////////////////////////////////////////////////////////////
	//	重载运算符，仅限于文本写入，不更新文件名
	CLogFile &operator<<(const char *ps) //写文本，不刷新文件名
	{
		log_pass(ps);
		return *this;
	}
	CLogFile &operator<<(char *ps) //写文本，不刷新文件名
	{
		log_pass(ps);
		return *this;
	}
	CLogFile &operator<<(string &s) //写文本，不刷新文件名
	{
		log_pass(s);
		return *this;
	}
	CLogFile &operator<<(int d) //写文本，不刷新文件名
	{
		char sbuf[24];
		sprintf(sbuf,"%d",d);
		log_pass(sbuf);
		return *this;
	}
	//////////////////////////////////////////////////////////////////////
	//	按行记录日志
	void log(string s) //要修改字符串，不用引用
	{
		if(ts_fmt!="") //若需要写时间戳
		{
			s=sFormat("%s%s",date.Now(ts_fmt.c_str()).c_str(),s.c_str());
		}
		if(s.back()!='\n') s.push_back('\n'); //若没有换行就加一个换行
		write(s);
	}
	void log(const char *format,...);
};
extern CLogFile slog;//系统的日志对象
#pragma pack(1)
typedef struct //8 Byte
{
	u8 syn; 	//0xA0 同步字
	u8 type		:1; //类型，0文本，1二进制
	u8 res		:3; //
	u8 vir		:4; //虚拟信道号（高4bit）
	u16 len; 	//本行数据长度
	u32 ms; 	//距离文件起始的ms数
} CMLOG_ROWHEAD; //混合日志行头
#pragma pack()
class CCMLog : public CLogFile //cmlog混合日志格式
{
public:
	u32 pre_ms; //上次记录的ms值
	u32 st_ms; //开始记录时的ms值
	u8 type_tab[16]={0}; //各信道的类型，0文本1二进制
	//此接口专门用来做0信道，文本信道
	virtual void log_pass(string &s)//直接写日志，文本
	{
		if(is_stdout) cout<<s; //这里是文本单独的处理，考虑输出到stdout
		logb((u8*)s.c_str(),s.size(),0); //二进制和文本都可以
	}
	virtual void logb(u8 *p,int n,int vir) //按指定的信道记录
	{
		logb_to_buf(p,n,vir,[this](u8 *p,int n,int end)
		{
			CLogFile::log_pass(p,n);
		});
	}
	//按指定的信道记录到缓存,输入缓存函数。
	//	缓冲函数参数为：数据指针，数据长度，是否是包结束
	void logb_to_buf(u8 *p,int n,int vir, function<void (u8 *,int,int)> cb)
	{ //拆分为帧
		while(n>0)
		{
			CMLOG_ROWHEAD th;
			th.syn=0xa0;
			th.type=type_tab[vir];
			th.vir=vir;
			int rec_len = n <= (65536-8) ? n : (65536-8); //本次实际要写入的数据长度,为了总长64K以内，稍微小一点
			th.len = rec_len;
			th.ms=com_time_getms()-st_ms;
			cb((u8*)&th,sizeof(th),0);
			cb(p,rec_len,1); //一包结束
			n -= rec_len;
			p+=rec_len;
		}
	}
};

class Exception : public exception //自定义的异常类
{
public:
	string errstr; //错误字符串
	Exception(string s)
	{
		errstr=s;
	}
	Exception(string s,int l) //支持文件名+行号的形式
	{
		ostringstream s1;
		s1<<s<<":"<<l;
		errstr=s1.str();
	}
	Exception &operator<<(const char *s)
	{
		errstr+=s;
		return *this;
	}
	Exception &operator<<(string &s)
	{
		errstr+=s;
		return *this;
	}
	virtual const char* what() const throw()
	{
		return errstr.c_str();
	}
};

#define D(A)	slog<<#A<<"\n";	A
#define DBGL	slog<<__FILE__<<":"<<__LINE__<<"\n"
#define DBG_OUT(x)	slog.log_pass(x)
#define THROW	throw Exception(__FILE__,__LINE__) //可以再 <<":err"

/////////////////////////////////////////////////////////////////////////////////
//5、数值工具
template<typename T>
T com_limit(T d,T min,T max) //限制极值
{
	if (d<min)
	{
		d=min;
	}
	else if (d>max)
	{
		d=max;
	}
	return d;
}
/////////////////////////////////////////////////////////////////////////////////
//6、事件驱动
class CEventTask //事件驱动任务
{
public:
	function<void (void *)> func; //任务函数
	void *para; //参数
	bool is_complete=false; //是否执行完成了？
};
class CDispatcher //调度器
{
public:
	queue<shared_ptr<CEventTask> > task_que; //任务队列
	int max_tasks=10000; //最大任务数
	mutex que_lock; //任务队列锁
	condition_variable event_sig; //事件信号
	condition_variable cmplt_sig; //完成信号
	CDispatcher() { }
	CDispatcher(const CDispatcher &d)=delete; //禁止移动构造
	void run(void) //运行函数，死循环，由外部开线程调用
	{
		while(1)
		{
			shared_ptr<CEventTask> pt; //临时指针
			{ //等事件信号
				unique_lock<mutex> lk(que_lock);
				event_sig.wait(lk,[this](){return !task_que.empty();});
				if(task_que.empty()) continue; //保险
				pt=task_que.front(); //若空时访问，会崩
				task_que.pop(); //去掉此任务
				if(!pt) continue;
			}
			pt->func(pt->para); //调用
			pt->is_complete=true;
			//唤醒等待线程
			cmplt_sig.notify_all();
		}
	}
	shared_ptr<CEventTask> BeginInvoke(function<void (void *)> func,void *para=0) //为调度器下达任务
	{
		shared_ptr<CEventTask> pt=make_shared<CEventTask>();
		pt->func=func;
		pt->para=para;
		{ //加入此任务
			unique_lock<mutex> lk(que_lock);
			if(task_que.size()>max_tasks) return shared_ptr<CEventTask>(0); //若失败
			task_que.push(pt);
		}
		//开始执行
		event_sig.notify_all();
		return pt;
	}
	//shared_ptr<CEventTask> BeginInvoke(void (*func)(void *),void *para=0) //函数指针需要重载，但影响lambda
	void wait(shared_ptr<CEventTask> t) //等待此任务执行完成
	{
		{ //等事件信号
			unique_lock<mutex> lk(que_lock);
			cmplt_sig.wait(lk,[t,this](){return t->is_complete;});
		}
	}
	void Invoke(function<void (void *)> func,void *para=0) //为调度器下达任务
	{
		auto v=BeginInvoke(func,para);
		if(v) wait(v);
	}
};
/////////////////////////////////////////////////////////////////////////////////
//7、python扩展
#ifdef PYEXT
#include "Python.h"
class CPyExt //需要单例，且只能用单线程调用
{
public:
	CPyExt(){}
	~CPyExt(){}
	void start(void); //开始Python解释器
	void end(void); //结束Python解释器
	int set_string(const char *key,const char *val); //设置变量值，适合变量文本大的情况
	string get_string(const char *key); //获取字符型变量值
	int eval(const char *p); //执行脚本字符串

	PyObject *p_main_Module;  //主模块
};
extern CPyExt pyext;
#endif

#endif

