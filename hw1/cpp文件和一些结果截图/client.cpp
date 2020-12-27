#include<iostream>
#include<winsock.h>
#include<string>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)
using namespace std;

void initialization();
void receive(SOCKET s_server);//接收消息函数，在线程中使用

int main()
{
    //定义服务端套接字,服务端地址
    SOCKET s_server;
    SOCKADDR_IN server_addr;
    initialization();
    //填充服务端信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(1234);
    //创建套接字
    s_server = socket(AF_INET, SOCK_STREAM, 0);

    /*
    connect()函数的第一个参数即为欲建立连接的本地套接字描述符，
    第二参数为服务器的socket地址，第三个参数为socket地址的长度，
    客户端通过三次握手来建立与TCP服务器的连接。
    */
    if (connect(s_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        cout << "服务器连接失败！" << endl;
        WSACleanup();
    }
    else
    {
        cout << "服务器连接成功！\n" << endl;

        //cout << "请选择注册（输入1） or 登录（输入2）:" << endl;
        //char choice[10]; cin >> choice;char strtmp[10];// cin.getline(strtmp, 10);
        //if (send(s_server, choice, 10, 0) < 0)
        //{
        //    cout << "发送失败！" << endl;
        //}


        cout << "请输入用户名:" << endl;
        char send_buf[100]; cin >> send_buf; 
        //---------cin之后使用getline会出问题，需要处理一下！！！--------------
        //string strtmp; getline(cin, strtmp); //strtmp 只是把缓存区清空，没有其他的作用
        char strtmp[10]; cin.getline(strtmp, 10);//strtmp 只是把缓存区清空，没有其他的作用
        //---------------------------------------------------
        if (send(s_server, send_buf, 100, 0) < 0)
        {
            cout << "发送失败！" << endl;
        }
        ////接收服务器反馈，是否注册or登录成功？
        //char recv_buf_begin[100];
        //if (recv(s_server, recv_buf_begin, 100, 0) < 0)
        //{
        //    cout << "接收失败！" << endl;
        //}



        cout << "开始聊天吧！o(≧▽≦)o" << endl;
        cout << "消息发送格式：\n私聊：message@user_name\n群聊：message@all 或 message@所有人" << endl;
        cout << "创建新的群聊方式：\n输入：#group_name,user_name1,user_name2..." << endl;
        cout << "输入exit退出\n" << endl;
    }

    //发送,接收数据
    /*
    int send(int sockfd, const void *msg, int len, int flags);
    int recv(int sockfd, void *buf, int len, unsigned int flags);
    send()中sockfd是你想发送数据的套接字描述字，msg 是指向你想发送的数据的指针，
    len是数据的长度，flags一般设置为0。

    recv()中sockfd是要读的套接字描述字，buf是要读的信息的缓冲，
    len是缓冲的最大长度，flags一般设置为0。
*/


    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)receive, (LPVOID)s_server, NULL, NULL);
    /*CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)sendmsg, (LPVOID)s_server, NULL, NULL);*/
    while (1)
    {
        cout << ">>>";
        char send_buf[100]; cin.getline(send_buf, 100);
        if (!strcmp(send_buf,"exit"))
        {
            return 0;
        }
        int send_len = send(s_server, send_buf, 100, 0);
        if (send_len < 0)
        {
            cout << "发送失败！" << endl;
        }
    }

    //关闭套接字
    closesocket(s_server);
    //释放DLL资源
    WSACleanup();
    return 0;
}
void initialization() {
    //初始化套接字库
    WORD w_req = MAKEWORD(2, 2);//版本号
    WSADATA wsadata;
    int err;
    err = WSAStartup(w_req, &wsadata);
    if (err != 0) {
        cout << "初始化套接字库失败！" << endl;
    }
    else {
        cout << "初始化套接字库成功！" << endl;
    }
    //检测版本号
    if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2)
    {
        cout << "套接字库版本号不符！" << endl;
        WSACleanup();
    }
    else {
        cout << "套接字库版本正确！" << endl;
    }
}

void receive(SOCKET s_server)
{
    while (1)
    {
        char recv_info[100];
        int recv_len = recv(s_server, recv_info, 100, 0);
        if (recv_len > 0)
        {
            string recv_buf = recv_info;
            int i = 0;
            for (i = recv_buf.length() - 1; recv_buf[i] != '#' && i > 0; i--)
            {
                ;
            }
            if (i == 0)
                cout << "消息格式错误！\n";
            string send_name = recv_buf.substr(i + 1, recv_buf.length() - 1);
            string recv_msg = recv_buf.substr(0, i);
            cout << endl << "----------------------------\n" <<
                send_name << ":" << recv_msg << "\n----------------------------\n";
            cout << ">>>";
        }

    }


}



