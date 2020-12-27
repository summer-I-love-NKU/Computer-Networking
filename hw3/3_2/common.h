#include<iostream>
#include<fstream>
#include<string>
#include<queue>
#include<WinSock2.h>//基于套接字，网络编程接口库文件
#include <time.h>//计时函数
#include<Windows.h>//Sleep
#include <direct.h>//getcewd()
#include<stdlib.h>//srand
#pragma comment(lib, "ws2_32.lib")//加载动态链接库
#pragma warning(disable:4996)//忽略警告，如新旧函数（inet_ntop）的检测
using namespace std;
/************************************************************/

#define SEND_P 70//发送/丢包概率
#define SEND_TIME_DELAY 200//发送延时
int nNetTimeout=500;//超时时间

/*******************************************************/
typedef unsigned short ushort;//给unsigned short起个别名
typedef unsigned int uint;

#define server_Port 1002
#define server_IP "127.0.0.1"
#define client_Port 1001
#define client_IP "127.0.0.1"
//语句替换！！！
#define SERVER_SEND sendto(socket_server, (char*)&sendbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, sizeof(SOCKADDR));Sleep(SEND_TIME_DELAY)
#define CLIENT_SEND sendto(socket_client, (char*)&sendbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));Sleep(SEND_TIME_DELAY)
#define RANDOM_EXPR rand()%101<SEND_P
/******************************************************************************/
//设置地址//recvfrom函数的最后一个参数，这个参数，必须 & ,不能int*  ???
SOCKADDR_IN client_addr, server_addr;
int recv_para_len=sizeof(SOCKADDR);
//文件名
string filename_list[6]={"1.txt","1.jpg","2.jpg","3.jpg","helloworld.txt","1.exe"};
//缓冲区设置的大一点，非阻塞的超时时间设置的短一点，文件就传的很快

clock_t _start, _end;
//当前连接状态
bool status;
/*******************************************************/
//缓冲区大小
#define BUF_LEN 65310//2**16==65536
#define HEADER_LEN 10//头部长度
#define DATA_LEN 65300
/*******************************************************/
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
struct PACKAGE
{
	ushort SEQ;//序列号 2bytes,16bits 0-65535
	ushort ACKnum;//确认序号
	ushort CheckSum;//检验和
	ushort ID;//多个标志位
	ushort Length;//数据长度，这是基本固定的
	char data[DATA_LEN];
};
#define SYN 0x1//建立连接
#define ACK 0x2//确认
#define FIN 0x4//断开连接
/*******************************************************/
/**GBN滑动窗
 * seq:16 0-15 
 * window:15
 * */
#define SEQ_SIZE 65500//0 1 2 ...15
// #define WINDOW_WIDTH 20 //GBN<=2**n-1 SR<=2**(n-1)
queue<PACKAGE> Send_Buf_Queue;
int p_Base,p_NextSeqNum;
/*******************************************************/
/**
 * 
 实验3-2进行了较大改动，协议更加完善清晰（接近TCP），数据报的结构更容易处理（结构体而不是字符数组，虽然字符数组更简单更易于传送，但编程操作上可能更加困难）
 实际上就是把原来的字符数组（头部+数据部分）改写成结构体PACKAGE（仍是10字节的头部+数据部分，占用字节数不变，但操作变得简单，可以直接对结构体属性赋值）。
 属性类型全部改用unsigned short,表示0-65535范围的无符号整数

TCP是字节流传送（序列号每次增加的值为字节数），本协议简单地使用序列号递增模式。
具体规则：
SEQ每次递增1（也是接收到的ACKnum），ACKnum是对方序列号加1（表示期待对方下次发送的序列号）
即：send_SEQ=recv_ACKnum,send_ACKnum=recv_SEQ+1

a为客户端（发送）b为服务端（接收），下面是交互过程的简单描述

ACK一直置位为1，其实没什么用。SYN和FIN只在开始和结束时使用。'.'代表这一位还未使用过（默认初始化为0）或不考虑

建立连接(假设初始X=0,Y=0)，TCP是随机序列号，这里从0开始。
a->b:SYN=1  ACK=.  SEQ=X=0  ACKnum=.
b->a:SYN=1  ACK=1  SEQ=Y=0  ACKnum=X+1=1
a->b:SYN=0  ACK=1  SEQ=1    ACKnum=1

数据传输：
a->b:SYN=1  ACK=.  SEQ=X=0  ACKnum=.
b->a:SYN=1  ACK=1  SEQ=Y=0  ACKnum=X+1=1
a->b:SYN=0  ACK=1  SEQ=1    ACKnum=1


断开连接(假设初始X=1,Y=1)，没有模仿TCP四次挥手https://www.jianshu.com/p/cd801d1b3147，这里三次交互就够。
a->b:FIN=1  ACK=1  SEQ=X=1  ACKnum=1
b->a:FIN=1  ACK=1  SEQ=Y=1  ACKnum=2
a->b:FIN=.  ACK=1  SEQ=1    ACKnum=1

b接收到FIN，也发送FIN（让对方知道自己知道了要断开连接），收到ACK，然后就退出。
a发送FIN,再收到FIN,发送ACK（这里对方收不到可以重传？），然后退出。


第一次挥手：客户端发送一个FIN=M，用来关闭客户端到服务器端的数据传送，客户端进入FIN_WAIT_1状态。意思是说"我客户端没有数据要发给你了"，但是如果你服务器端还有数据没有发送完成，则不必急着关闭连接，可以继续发送数据。
第二次挥手：服务器端收到FIN后，先发送ack=M+1，告诉客户端，你的请求我收到了，但是我还没准备好，请继续你等我的消息。这个时候客户端就进入FIN_WAIT_2 状态，继续等待服务器端的FIN报文。
第三次挥手：当服务器端确定数据已发送完成，则向客户端发送FIN=N报文，告诉客户端，好了，我这边数据发完了，准备好关闭连接了。服务器端进入LAST_ACK状态。
第四次挥手：客户端收到FIN=N报文后，就知道可以关闭连接了，但是他还是不相信网络，怕服务器端不知道要关闭，所以发送ack=N+1后进入TIME_WAIT状态，如果Server端没有收到ACK则可以重传。服务器端收到ACK后，就知道可以断开连接了。客户端等待了2MSL后依然没有收到回复，则证明服务器端已正常关闭，那好，我客户端也可以关闭连接了。最终完成了四次握手。

*/