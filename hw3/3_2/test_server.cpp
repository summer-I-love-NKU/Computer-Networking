#include "./common.h"
int send_Ret,recv_Ret;//bytes数返回值
PACKAGE recvbuf,sendbuf;//结构体也能发送！！！ (char*)&sendbuf!!!!
int WINDOW_WIDTH;
ofstream res;
/*******************************************************************/
void makePackage();
bool doCheckSum();
bool IsSYN();
bool IsFIN();
bool CheckSEQ_EXPECTED();
// int GetSEQ(){return recvbuf.SEQ;}
/*******************************************************************/
int main()//int argc,char* argv[]) //命令行执行传参方法！！！
{
cin>>WINDOW_WIDTH;// WINDOW_WIDTH=atoi(argv[1]);
LABEL_start:
Sleep(1000);status=false;recv_Ret=-1;//算是一种清空缓存吧
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
	SOCKET socket_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

// 设置发送超时//设置接收超时
setsockopt(socket_server, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
setsockopt(socket_server, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
//srand()函数产生一个以当前时间开始的随机种子
srand((unsigned)time(NULL)); 

	while (1)
	{
		recv_Ret = recvfrom(socket_server, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
		if (recv_Ret <= 0)
		{
			// cout << "没有收到连接请求！" << endl;
			cout << "no connect request！" << " ";
		}
		else
		{
			if (IsSYN())
			{
LABEL_CONNECT:
				/*****************************************连接成功**************************************************************/
				cout << "connect!!!" << endl;// cout << "收到SYN请求，连接成功！" << endl;
				status = true;
				makePackage();
				SERVER_SEND;

				recv_Ret=-1;//防止收不到文件名就过快执行下面（也能执行）
				//--------------------------文件名-----------------------
				do
				{
					recv_Ret = recvfrom(socket_server, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
				} while (recv_Ret < 0);
				string file_name=string(recvbuf.data);
				cout << "接收到文件名: "<<file_name<< endl;
				//告知对方接收到文件名，不占用序列号（不调用makepackage函数,sendbuf一定有上次的ACK，这就够了）
				SERVER_SEND;
				//--------------------------------------------------
				fstream file;
				file.open(file_name, ios::app | ios::out | ios::binary);

				//记录第几次接收数据包
				int trans_bytes = 0;
				_start = clock();
				while (1)
				{
LABEL_RECV:
					recv_Ret = recvfrom(socket_server, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
					if (recv_Ret <= 0)
					{
						cout << "no data!" << " ";
					}
					else
					{
						if (doCheckSum())
						{
								if(CheckSEQ_EXPECTED())
								{
				/**********************************************校验和正确，写入文件**************************************************************/
									if (IsFIN())
									{
										cout << "successfully recv " << recvbuf.SEQ << endl;
										//cout << "第" << recvbuf.SEQ << "次收到数据！"<< "校验和正确！！！" << endl;//从1开始
										trans_bytes += recvbuf.Length;
										file.write(recvbuf.data, recvbuf.Length);
										file.close();

										_end = clock();
										double trans_time = (double)(_end - _start) / CLOCKS_PER_SEC;
										cout << "文件 "<<file_name<<" 传输完成！！！" << endl;
										cout<<"\n窗口大小"<<WINDOW_WIDTH<<endl;
										cout << "传输用时:" << trans_time << "秒" << endl;
										cout << "传输数据量：" << trans_bytes << " bytes ( " << trans_bytes / 1024 << "KB )" << endl;
										cout<<"吞吐率："<<trans_bytes/trans_time<<" bytes/s ( "<<trans_bytes/1024/trans_time<<"KB/s )"<<endl;
										
										res.open("res.txt",ios::app);
										res<<WINDOW_WIDTH<<endl<<trans_time<<endl<<trans_bytes/1024/trans_time<<endl<<endl<<endl;
										res.close();
										
										//发送FIN信号
										makePackage();
										sendbuf.ID |= FIN;
// 										if(RANDOM_EXPR)//即遇到rand()%RAND_MOD_NUM==0丢弃
// 										{
// 											SERVER_SEND;
// 										}
// 										else
// 										{
// LABEL_ATTENTION_has_problem:
// 											SERVER_SEND;
// 											/**
// 											 这样后果就很严重了，先不试了！！！！！
// 											 * */
// 											cout<<"****************************随机丢弃挥手的FIN数据包！！！！！****************************"<<endl;
// 										}
										
										/**
										 有问题，
										 发送端发来了FIN
										 但发送端可能收不到FIN，那就再发一次FIN，等到发送端知道了，发送ACK（发送端只有这时候发送ACK），
										 之后再退出

										 * */
										//int check_know_FIN=0;//while循环超过一定次数就直接退出。
										recvbuf.ID=0;//初始化清空一下
										while(recvbuf.ID&ACK==0||recvbuf.ID&FIN==0)//&&check_know_FIN<3000)//未收到ACK
										{
											cout<<"send FIN!"<<" ";
											// check_know_FIN++;
											SERVER_SEND;//含有FIN
											do
											{
												recv_Ret = recvfrom(socket_server, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
											} while (recv_Ret < 0);
										}
										
										goto LABEL_end;
									}
									else
									{
										cout << "successfully recv " << recvbuf.SEQ << endl;
										//cout << "第" << recvbuf.SEQ << "次收到数据！"<< "校验和正确！！！" << endl;//从1开始
										trans_bytes += recvbuf.Length;

										file.write(recvbuf.data, recvbuf.Length);
									/****************************随机丢弃ACK包！！！！***************************************************/
										makePackage();
										if(RANDOM_EXPR)//即遇到rand()%RAND_MOD_NUM==0丢弃
										{
											SERVER_SEND;
										}
										else
										{
											cout<<"**********************随机丢弃ACK包！！！！！**********************"<<endl;
										}
										
										continue;
									}

								}
								else
								{
									/*******************************收到失序(超前)的包************************************************
									 * 比如收到了seq5，回复acknum==6，期望的是seq6
									 * 结果收到seq8，直接丢弃！！！
									 * 不再调用makepackage函数（它就是ack置位，把acknum写成接收的seq+1），
									 * package.ACKnum不变
									 * 所以头部不变！！！！
									 * 
									 * 要是收到seq3呢，更是直接发ack6
									 * **/
									cout<<"接收到失序的数据包 "<<recvbuf.SEQ<<" ,丢弃，发送原ACKnum: "<<sendbuf.ACKnum<<"!!!"<<endl;
									if(1)//即遇到rand()%RAND_MOD_NUM==0丢弃
									{
										SERVER_SEND;
									}
									else
									{
										cout<<"**********************随机丢弃ACK包（收到失序数据包时的ACK）！！！！！**********************"<<endl;
									}
									continue;
								}
								
							
							
						}
						else
						{
							cout << "数据包损坏，校验和错误！" << endl;//包损坏！！！
							//不发送包，相当于没接收等待重传即可！！！
							continue;
						}
					}
				}
			}
		}
	}

LABEL_end:
	//关闭socket连接//清理
	closesocket(socket_server);WSACleanup();
	cout << "\n**************完成一次测试！！！**************\n\n\n" << endl;
	//goto LABEL_start;//
LABEL_exit:
	//system("pause");
	return 0;
}

void makePackage()
{
	sendbuf.ID = ACK;//一直是ACK（定义时就写上ACK了）
	sendbuf.ACKnum=recvbuf.SEQ+1;
	sendbuf.Length=recvbuf.Length;
}
//对接收的数据计算校验和，不取反，结果为0xFFFF，说明数据正确
bool doCheckSum()
{
	//数据部分
	int count = recvbuf.Length / 2 + bool(recvbuf.Length % 2);
	ushort *array = new ushort[count+HEADER_LEN/2];
	if (recvbuf.Length % 2 == 0)
	{
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &recvbuf.data[i * 2], 2);
			//(void *)???
		}
	}
	else
	{
		for (int i = 0; i < count-1; i++)
		{
			memcpy(&array[i], &recvbuf.data[i * 2], 2);
		}
		array[count-1] = (ushort)recvbuf.data[recvbuf.Length-1];
	}
	//头部部分
	array[count]=recvbuf.SEQ;
	array[count+1]=recvbuf.ACKnum;
	array[count+2]=recvbuf.CheckSum;
	array[count+3]=recvbuf.ID;
	array[count+4]=recvbuf.Length;

	count+=HEADER_LEN/2;uint sum = 0;
	while (count--)
	{
		sum += array[count];//注意啦count--
		// cout<<array[count]<<endl;
		sum = (sum >> 16) + (sum & 0xFFFF);
	}
	// cout<<"校验和："<<sum<<endl;
	if (sum == 0xFFFF)
		return true;
	else
		return false;
}

bool IsSYN()
{
	if ((recvbuf.ID & SYN) != 0) 
		return true;
	else
		return false;
}
bool IsFIN()
{
	if ((recvbuf.ID & FIN) != 0) 
		return true;
	else
		return false;
}

//接收方处理时要能处理比期望seq小的seq号？？？？啥意思老注释
bool CheckSEQ_EXPECTED()
{
	if (recvbuf.SEQ == sendbuf.ACKnum)//初始acknum==1，先收到seq1，（握手用了seq0）
		return true;
	else
		return false;
}
