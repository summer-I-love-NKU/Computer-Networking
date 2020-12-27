#include "./common.h"
int send_Ret, recv_Ret;
char sendbuf[HEADER_LEN];
char recvbuf[BUF_LEN];
char sendbuf2[BUF_LEN]={'9','9','9',0};
char sendbuf3[BUF_LEN]={'9','9',0};
char sendbuf4[BUF_LEN]={'9',0};
void f();
int a=0;
/*******************************************************************/
int main()
{

#if 1
	//WinSocket的初始化
	WSAData wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		cout << "WSAStartup Error = " << WSAGetLastError() << endl;
		return 0;
	}

	//创建socket，服务器的套接字
	//地址类型：AF_INET
	//服务类型：SOCK_DGRAM， UDP类型，不保证数据接收的顺序，非可靠连接
	//协议：UDP，若为0则由系统自动选择
	socket_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_server == SOCKET_ERROR)
	{
		cout << "socket Error = " << WSAGetLastError() << endl;
		return 0;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_Port);
	server_addr.sin_addr.s_addr = inet_addr(server_IP);

	//绑定服务端地址到socket
	if (bind(socket_server, (SOCKADDR *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		cout << "bind Error = " << WSAGetLastError() << endl;
		return 0;
	}

	//也要设置客户端的地址端口号
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(client_Port);
	client_addr.sin_addr.s_addr = inet_addr(client_IP);

#endif

int a=0;
	//设置发送超时
	setsockopt(socket_server, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
	//设置接收超时
	setsockopt(socket_server, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));

int k=4;
	while (k--)
	{
		recv_Ret = recvfrom(socket_server, recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
		//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)f, NULL, NULL, NULL);
		cout<<recvbuf<<endl;
	}

	system("pause");
}

void f()
{
	if (recv_Ret <= 0)
		{
			cout << "没有收到数据！" << endl;
		}
	else
	{
		
		cout<<"收到！！"<<recvbuf<<endl;
	
		if(a==0)
		{
			sendto(socket_server,sendbuf2, HEADER_LEN, 0, (SOCKADDR*)&client_addr, sizeof(SOCKADDR));
			a=1;
		}
		else if(a==1)
		{
			sendto(socket_server,sendbuf3, HEADER_LEN, 0, (SOCKADDR*)&client_addr, sizeof(SOCKADDR));a=2;
		}
		else
		{
			
			sendto(socket_server,sendbuf4, HEADER_LEN, 0, (SOCKADDR*)&client_addr, sizeof(SOCKADDR));
		}
		
		
	}

}