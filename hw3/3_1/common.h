#include<iostream>
#include<fstream>
#include<string>
// #include<stdio.h>
#include<WinSock2.h>//基于套接字，网络编程接口库文件
#include <time.h>//计时函数
// #include <assert.h>
#include <direct.h>//getcewd()
#include<stdlib.h>//srand
#pragma comment(lib, "ws2_32.lib")//加载动态链接库
#pragma warning(disable:4996)//忽略警告，如新旧函数（inet_ntop）的检测
using namespace std;

#define server_Port 1001
#define server_IP "127.0.0.1"
#define client_Port 1002
#define client_IP "127.0.0.1"
/******************************************************************************/
//设置地址
SOCKADDR_IN client_addr, server_addr;
//recvfrom函数的最后一个参数，这个参数，必须 & ,不能int*  ???
int recv_para_len=sizeof(SOCKADDR);

////判断是否发送或者接收成功
// void mycheck(int sendto_or_recvfrom)
// {
//     if (sendto_or_recvfrom == SOCKET_ERROR) 
//     {
//         cout << "Error!!!" << WSAGetLastError() << endl;
//         return;
//     }
// }
//文件名字//0 1 2 3 4 5
string filename_list[6]={"1.txt","1.jpg","2.jpg","3.jpg","helloworld.txt","1.exe"};

//缓冲区设置的大一点，非阻塞的超时时间设置的短一点，文件就传的很快
int RAND_MOD_NUM=15;//
int nNetTimeout=5;//ms
// // 设置发送超时
// setsockopt(socket_client,SOL_SOCKET,SO_SNDTIMEO, (char *)&nNetTimeout,sizeof(int));
// //设置接收超时
// setsockopt(socket_client,SOL_SOCKET,SO_RCVTIMEO, (char *)&nNetTimeout,sizeof(int));

/*******************************************************/
#define BUF_LEN 65310//2**16-6
#define HEADER_LEN 10//头部长度
#define DATA_LEN 65300

/*  头部的设计
	Header:10 bytes
	| 2 bytes  | 2 bytes  | 2 bytes  | 2 bytes  |  2 bytes  |
	|   SEQ    |  ACKnum  | CheckSum |	  ID	|   Length  |

	
	ID:2 bytes(只使用最低三位)
	|   FIN  |  ACK  |  SYN  |

	
		// char SendBuf[20];char c[10]="12345";
		// header.SEQ = 48;//0
		// header.ACKnum = 49;//1
		// header.CheckSum = 0x30;//0
		// header.ID = 0x31;//1 注意是先给高位赋值！！！！！
		// header.Length = 0b0011000100110001;//11
		// memcpy(&SendBuf, &header, 10);memcpy(&SendBuf[10], &c, 3);
		// for (int i = 0; i < 20; i++)
		// {
		//    cout<<SendBuf[i];
		//    //输出：
		//    //0a1a0a1a11123aaa... (a为乱码)
		// }
*/
struct HEADER
{
	short SEQ;//序列号
	short ACKnum;//确认序号
	short CheckSum;//检验和
	short ID;//多个标志位
	short Length;//数据长度，这是基本固定的
};
#define SYN 0x1//建立连接
#define ACK 0x2//确认
#define FIN 0x4//断开连接
#define ID_bit 0x6
#define ACKnum_bit 0x2
#define SEQ_bit 0x0
#define Length_bit 0x8



