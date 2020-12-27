#include"./common.h"
int send_Ret,recv_Ret;//字节数返回值
 char recvbuf[HEADER_LEN];
//sendbuf： 低十位是头部，后面是数据
//recvbuf： 只有十位，接收头部信息即可，（对于发送端）
HEADER header;
//当前连接状态

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

    //创建socket，客户端的套接字
        //地址类型：AF_INET
        //服务类型：SOCK_DGRAM， UDP类型，不保证数据接收的顺序，非可靠连接
        //协议：UDP，若为0则由系统自动选择
    SOCKET socket_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socket_client == SOCKET_ERROR) 
    {
        cout << "socket Error = " << WSAGetLastError() << endl;
        return 0;
    }
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(client_Port);
    client_addr.sin_addr.s_addr = inet_addr(client_IP);
    
    //绑定客户端地址到socket
    if (bind(socket_client, (SOCKADDR*)&client_addr, sizeof(client_addr)) == SOCKET_ERROR) 
    {
        cout << "bind Error = " << WSAGetLastError() << endl;
        return 0;
    }
    
    //也要设置服务端的地址端口号
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_Port);
    server_addr.sin_addr.s_addr = inet_addr(server_IP);

#endif

// 设置发送超时
setsockopt(socket_client,SOL_SOCKET,SO_SNDTIMEO, (char *)&nNetTimeout,sizeof(int));
//设置接收超时
setsockopt(socket_client,SOL_SOCKET,SO_RCVTIMEO, (char *)&nNetTimeout,sizeof(int));


queue<char*> a;
char q1[5]={'1','2',0};char q2[5]={'1','2','3',0};char q3[5]={'9','9',0};
    a.push(q1);a.push(q2);a.push(q3);
char qq[10];
char ww[10];

/////////////////////////////////////////////////
memcpy(&qq,a.front(),10);
a.pop();
memcpy(&ww,a.front(),10);
//--------------------------------!!!!!!!!!!!!成功！！！！！！！！！！！！memcpy的问题！！为什么还是不懂！！！！



    cout<<qq<<endl;
    cout<<ww<<endl;

	sendto(socket_client,qq, HEADER_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
    sendto(socket_client,ww, HEADER_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
    
while (1) 
{
	
	recv_Ret = recvfrom(socket_client, recvbuf, HEADER_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
	if (recv_Ret <= 0)
	{
	}
    else
    {
        cout<<"收到！！"<<recvbuf<<endl;
    }

}


	closesocket(socket_client);WSACleanup();
	system("pause");
}

