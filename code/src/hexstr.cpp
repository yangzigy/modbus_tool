
/*
文件名：hexstr.cpp
作者：3817
时间：2014-12-04 15:53:03.039133
功能：

*/

#include "hexstr.h"
#include "ctype.h"


s8 char2bin(const char c)	//一个字符转数字
{
	if (c>='0' && c<='9')
	{
		return c-'0';
	}
	else if (c>='a' && c<='f')
	{
		return c-'a'+10;
	}
	else if (c>='A' && c<='F')
	{
		return c-'A'+10;
	}
	return -1;
}

//二进制数据转16进制字符的
//输入二进制数据，二进制数据的个数，输出字符缓冲（调用者提供）
u32 bin2str(u8 *p,int n,char *out)//返回字符缓存使用量
{
	int a=0;
	for (int i = 0; i < n; ++i)
	{
		a+=sprintf(out+a,"%02X",p[i]);
	}
	return a;
}

//hex字符串转二进制数据 02ab 3af4b5 0d 数据无论是否分组，都按内存从低到高
//以空格分隔，若没有空格分隔，要求必须以两个字符为一组。
//输入字符串，字符串的大小，输出二进制缓冲要求调用者提供
u32 str2bin(const char *str,int n,vector<u8> &v)//0成功，非零错误号
{
	int stat=0;//状态，0：找字符；1：找第二个字符；
	int i=0;//字符计数
	u8 t;
	while(n--)
	{
		char c=*str++;
		i++;
		s8 tem=char2bin(c);
		if (tem==-1)//判断是否为非法字符
		{
			if (c==' ')
			{
				if (stat!=0)	//若空格前有单个字符，也要输出
				{
					v.push_back(t);
					stat=0;
				}
				continue;
			}
			return i;
		}
		switch(stat)
		{
		case 0:
			t=tem;
			stat=1;
			break;
		case 1:
			t*=16;
			t+=tem;
			v.push_back(t);
			stat=0;
		default:
			break;
		}
	}
	if (stat!=0)	//若最后一个是单个字符，也要输出
	{
		v.push_back(t);
	}
	return 0;
}
u32 str2bin(string &str,vector<u8> &v)
{
	return str2bin(str.c_str(),str.length(),v);
}
