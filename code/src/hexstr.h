
/*
文件名：hexstr.h
作者：3817
时间：2014-12-04 15:53:03.039133
功能：

*/
#ifndef HEXSTR_H
#define HEXSTR_H

#include "main.h"

using namespace std;

u32 bin2str(u8 *p,int n,char *out);//返回字符缓存使用量
u32 str2bin(const char *str,int n,vector<u8> &v);//0成功，非零错误号
u32 str2bin(string &str,vector<u8> &v);

#endif

