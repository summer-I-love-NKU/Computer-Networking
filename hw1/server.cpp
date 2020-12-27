#include<iostream>
#include<fstream>
#include<winsock.h>
#include<map>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)
using namespace std;

#define C_NUM 50//能连接的客户端数量
int c_num = 0;//当前客户端数量
map<string, SOCKET> client_info;//存储用户信息
map<SOCKET, string> client_info2;//存储用户信息，按不同顺序存两次方便查找
map<string, string> msg_for_absent;//缓存给未在线用户发送的信息
//准备创建的个性化群聊
//struct group_user
//{
//    string user[10];
//};
//map<string, group_user> group;

void initialization();//初始化套接字库
void communicate(SOCKET s_accept);//服务器的收发消息函数，在线程中使用


int main()
{
    //定义服务端套接字，接受请求套接字//服务端地址，客户端地址
    SOCKET s_server;
    SOCKET s_accept[C_NUM] = { 0 };
    SOCKADDR_IN server_addr;
    SOCKADDR_IN client_addr;
    //初始化套接字库
    initialization();

    //填充服务端信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(1234);
    //套接字命名，绑定socket和端口号
    s_server = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(s_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)//-1 
    {
        cout << "套接字绑定失败！" << endl;
        WSACleanup();
    }
    else
    {
        cout << "套接字绑定成功！" << endl;
    }

    //设置套接字为监听状态
    //s_server：本地已建立、尚未连接的套接字号，服务器愿意从它上面接收请求
    //SOMAXCONN：连接队列的最大长度，用于限制排队请求的个数
    if (listen(s_server, SOMAXCONN) < 0) //SOMAXCONN==5
    {
        cout << "设置监听状态失败！" << endl;
        WSACleanup();
    }
    else
    {
        cout << "设置监听状态成功！" << endl;
    }
    cout << "服务端正在监听连接，请稍候...." << endl << endl;


    //接收数据
    while (1)
    {
        //接受连接请求
        int len = sizeof(SOCKADDR);
        s_accept[c_num] = accept(s_server, (SOCKADDR*)&client_addr, &len);
        if (s_accept[c_num] == SOCKET_ERROR)
        {
            cout << "连接失败！" << endl;
            WSACleanup();
            return 0;
        }
        cout << "连接" << c_num + 1 << "建立，开始输入用户名" << endl;

    //label_recv_choice:
    //    char choice[10];
    //    if (recv(s_accept[c_num], choice, 10, 0) < 0)
    //    {
    //        cout << "接收失败！" << endl;
    //    }

    //    //先定义标签吧
    //label_recv_name:
        char recv_buf[100];
        if (recv(s_accept[c_num], recv_buf, 100, 0) < 0)
        {
            cout << "接收失败！" << endl;
        }
        else
        {
    //        //需要检查注册用户名，不允许重复！！！但是允许登录的用户名重复，覆盖map的键
    //        //注册
    //        if (!strcmp(choice, "1"))
    //        {
    //            if (client_info.count(recv_buf) != 0)//与现有的用户名重复了
    //            {
    //                char send_msg[100] = "用户名重复，请重新输入！\n";
    //                if (send(s_accept[c_num], send_msg, 100, 0) < 0)//告诉他重新注册用户名
    //                {
    //                    cout << "发送失败！" << endl;
    //                }
    //                goto label_recv_name;
    //            }
    //            else
    //            {
    //                char send_msg[100] = "用户名注册成功！\n";
    //                if (send(s_accept[c_num], send_msg, 100, 0) < 0)
    //                {
    //                    cout << "发送失败！" << endl;
    //                }
    //            }
    //        }
    //        //登录
    //        else if (!strcmp(choice, "2"))
    //        {
    //            char send_msg[100] = "登录成功！\n";
    //            if (send(s_accept[c_num], send_msg, 100, 0) < 0)
    //            {
    //                cout << "发送失败！" << endl;
    //            }
    //        }
    //        else
    //        {
    //            char send_msg[100] = "输入错误！请重新输入\n";
    //            if (send(s_accept[c_num], send_msg, 100, 0) < 0)
    //            {
    //                cout << "发送失败！" << endl;
    //            }
    //            goto label_recv_choice;
    //        }

            client_info[recv_buf] = s_accept[c_num]; client_info2[s_accept[c_num]] = recv_buf;
            cout << "用户登录成功，当前用户列表(用户名，SOCKET类型值)：" << endl;
            for (map<string, SOCKET>::iterator iter = client_info.begin(); iter != client_info.end(); iter++)
                cout << iter->first << "  " << iter->second << endl;
            cout << endl;

            //检查是否有人发过消息
            if (msg_for_absent.count(recv_buf) != 0)
            {
                char send_msg[100];strcpy(send_msg,msg_for_absent[recv_buf].c_str());
                if (send(client_info[recv_buf], send_msg, 100, 0) < 0)
                {
                    cout << "发送失败！" << endl;
                }
            }

        }

        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)communicate, (LPVOID)s_accept[c_num], NULL, NULL);


        c_num++;
    }
    //关闭套接字
    closesocket(s_server);
    closesocket(s_accept[C_NUM]);
    //释放DLL资源
    WSACleanup();
    return 0;
}

//初始化套接字库
void initialization()
{
    WORD w_req = MAKEWORD(2, 2);//版本号
    WSADATA wsadata;
    int err = WSAStartup(w_req, &wsadata);//成功返回0
    if (err != 0)
    {
        cout << "初始化套接字库失败！" << endl;
    }
    else
    {
        cout << "初始化套接字库成功！" << endl;
    }
    //检测版本号
    if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2)
    {
        cout << "套接字库版本号不符！" << endl;
        WSACleanup();
    }
    else
    {
        cout << "套接字库版本正确！" << endl;
    }
}


//在这里要有while循环！！
void communicate(SOCKET s_accept)
{
    while (1)
    {
        char recv_info[100];
        if (recv(s_accept, recv_info, 100, 0) > 0)
        {
          
            string recv_buf = recv_info;

            int i = 0;
            for (i = recv_buf.length() - 1; recv_buf[i] != '@' && i > 0; i--)
            {
                ;
            }
            
            //这是创建群聊的识别
            if (recv_buf[0] == '#')
            {
                int t1 = recv_buf.length();recv_buf=recv_buf.substr(1, t1);
                int a[10] = {-1}, k = 1;
                //#g,1,2,3  --->k=4=长度
                int the_len = recv_buf.length();
                for (int i = 0; i < the_len; i++)
                {
                    if (recv_buf[i]==',')
                    {
                        a[k] = i; 
                        k++;
                    }
                }
                string b[10];
                for (int i = 0; i < k-1; i++)
                {
                    b[i] = recv_buf.substr(a[i]+1,a[i+1]-a[i]-1);
                }
                b[k-1] = recv_buf.substr(a[k-1] + 1, 10);
                cout << "为这些用户创建群聊："<<b[0]<<endl;
                //group_user gu;
                for (int i = 1; i < k; i++)
                {
                    cout << b[i] << "  ";
                    //gu[i - 1] = b[i];
                }
               /* group[b[0]] = gu;

                for (map<string, SOCKET>::iterator iter = group.begin(); iter != group.end(); iter++)
                    cout << iter->first << "  " << iter->second << endl;*/

                continue;
            }
            //这句要在判断群聊的后面
            if (i == 0)
            {
                cout << "消息格式错误！" << endl;
                continue;
            }

            //解析接收到的消息，如A发的：hello@B  改写为 hello#A(A发的)， 并发送给B
            //s.substr(pos, n)返回一个string，包含s中从pos开始的n个字符的拷贝
            //（pos的默认值是0，n的默认值是s.size() - pos，即不加参数会默认拷贝整个s）
            //若pos的值超过了string的大小，则substr函数会抛出一个out_of_range异常；
            //若pos+n的值超过了string的大小，则substr会调整n的值，只拷贝到string的末尾,所以第二个参数可以超了
            string the_name = recv_buf.substr(i + 1,recv_buf.length()-1);
            string the_msg = recv_buf.substr(0, i);
            string msg_processed = the_msg + '#' + client_info2[s_accept];
            char send_msg[100]; strcpy(send_msg, msg_processed.c_str());

           if (the_name == "all" || the_name == "所有人")
           {
                for (map<string, SOCKET>::iterator iter = client_info.begin(); iter != client_info.end(); iter++)
                {
                    if (send(client_info[iter->first], send_msg, 100, 0) < 0)
                    {
                        cout << "给" << iter->first << "用户的消息发送失败！进行缓存" << endl;
                        msg_for_absent[iter->first] = send_msg;
                    }
                }
                cout << client_info2[s_accept] << " --> all :" << the_msg << endl;
                ofstream f;
                f.open("server_saved_message.txt", ios::app);
                f << client_info2[s_accept] << " --> all :" << the_msg << endl;
                f.close();
            }
            else
            {
                if (send(client_info[the_name], send_msg, 100, 0) < 0)
                {
                    cout << "给" << the_name << "用户的消息发送失败！进行缓存" << endl;
                    msg_for_absent[the_name] = send_msg;
                }
                cout << client_info2[s_accept] << " --> " << the_name << " :" << the_msg << endl;
                ofstream f;
                f.open("server_saved_message.txt", ios::app);
                f << client_info2[s_accept] << " --> " << the_name << " :" << the_msg << endl;
                f.close();
            }

        }

    }

}
