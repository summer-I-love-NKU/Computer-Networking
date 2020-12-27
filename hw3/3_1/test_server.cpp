#include "./common.h"
int send_Ret, recv_Ret;
char sendbuf[HEADER_LEN];
char recvbuf[BUF_LEN];
//sendbuf： 只有十位，发送头部信息
//recvbuf： 接收的数据
HEADER header;
//当前连接状态
bool status = false;
int fileID;
clock_t _start, _end;
/*******************************************************************/
void makePackage()
{
	if (!status) //刚刚开始连接
	{
		header.ID = ACK; //确认连接请求，ACK置位
		//header.SEQ = 0;//用不到
		//ACKnum（确认序号）等于接收的序列号
		memcpy(&header.ACKnum, &recvbuf[SEQ_bit], 2);
		memcpy(&sendbuf, &header, HEADER_LEN);
	}
	else
	{
		header.ID = ACK; //header.SEQ =(1+header.SEQ)%2;//不要用~！否则是-1！！//其实用不到
		//ACKnum（确认序号）等于接收的序列号
		memcpy(&header.ACKnum, &recvbuf[SEQ_bit], 2);
		memcpy(&sendbuf, &header, HEADER_LEN);
	}
}

//对接收的数据计算校验和，不取反，结果为0xFFFF，说明数据正确
bool doCheckSum()
{
	unsigned short *array;
	unsigned int sum = 0;
	int length = 0;
	memcpy(&length, &recvbuf[8], 2);
	int count = (length + 10) / 2;
	// cout<<length<<endl;

	if ((length + 10) % 2 == 0)
	{
		array = new unsigned short[count];
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &recvbuf[i * 2], 2);
		}
	}
	else
	{
		array = new unsigned short[count + 1];
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &recvbuf[i * 2], 2);
		}
		array[count] = (unsigned short)recvbuf[count * 2];
	}

	int i = 0;
	while (count--)
	{
		sum += array[i++];
		sum = (sum >> 16) + (sum & 0xFFFF);
	}
	// cout<<sum<<endl;
	if (sum == 0xFFFF)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//注意，判断条件是按位与不为0，而不能是按位异或等于0（因为会有别的位为1你管不了！！！）
bool IsSYN()
{
	short syn;
	memcpy(&syn, &recvbuf[ID_bit], 1);
	if ((syn & SYN) != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//判断是不是最后一个包的ACK
bool IsFIN()
{
	short fin;
	memcpy(&fin, &recvbuf[ID_bit], 1);
	if ((fin & FIN) != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//检查ACK的序列号SEQ正确与否,比如接收到包0，返回了acknum==0，这时候该接收包1了，
//再收到0就丢弃（比如发送的acknum==0丢失，对方又重传了包0）,但是还要返回acknum==0!!!
bool CheckSEQ_IS_NEW()
{
	short seq;
	memcpy(&seq, &recvbuf[SEQ_bit], 2);//SEQ字段
	if (seq + header.ACKnum==1) //0 1 0 1交替
    {
		return true;
	}
	else
	{
		return false;
	}
}

/*******************************************************************/
int main()
{
LABEL_start:
status=false;
memset(&recvbuf, 0, BUF_LEN); 
memset(&sendbuf, 0, HEADER_LEN); 
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

	// 设置发送超时
	setsockopt(socket_server, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
	//设置接收超时
	setsockopt(socket_server, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));

	srand((unsigned)time(NULL)); //srand()函数产生一个以当前时间开始的随机种子

	while (1)
	{
		recv_Ret = recvfrom(socket_server, recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
		if (recv_Ret <= 0)
		{
			cout << "没有收到数据！" << endl;
			continue;
		}
		else
		{
			if (IsSYN())
			{
				/*****************************************连接成功**************************************************************/
				cout << "收到SYN请求，连接成功！" << endl;
				status = true;
				makePackage();
				sendto(socket_server, sendbuf, HEADER_LEN, 0, (SOCKADDR *)&client_addr, sizeof(SOCKADDR));

				//--------------------------文件名-----------------------
				do
				{
					recv_Ret = recvfrom(socket_server, recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
				} while (recv_Ret < 0);
				memcpy(&fileID, &recvbuf[Length_bit], 2);
				cout << "接收到文件名！" << endl;
				//--------------------------------------------------
				fstream file;
				file.open(filename_list[fileID], ios::app | ios::out | ios::binary);
				//记录第几次接收数据包
				int index = 1, trans_bytes = 0, data_len = 0;
				_start = clock();
				while (1)
				{
					recv_Ret = recvfrom(socket_server, recvbuf, BUF_LEN, 0, (SOCKADDR *)&client_addr, &recv_para_len);
					if (recv_Ret <= 0)
					{
						cout << "没有收到数据！" << endl;
						continue;
					}
					else
					{
						if (doCheckSum())
						{
							if(CheckSEQ_IS_NEW())
							{
			/**********************************************校验和正确，写入文件**************************************************************/
								cout << "第" << index++ << "次收到数据！"<< "校验和正确！！！" << endl;
								
								if (IsFIN())
								{
									memcpy(&data_len, &recvbuf[Length_bit], 2);
									trans_bytes += data_len;
									// if (data_len >= 0 && data_len < DATA_LEN)
									// {
									file.write(&recvbuf[10], data_len);
									file.close();

									_end = clock();
									double endtime = (double)(_end - _start) / CLOCKS_PER_SEC;
									cout << "文件传输完成！！！" << endl;
									cout << "\n传输用时:" << endtime << "秒" << endl;
									cout << "传输数据量：" << trans_bytes << "字节 ( " << trans_bytes / 1024 << "KB )" << endl;

									//发送FIN信号
									makePackage();
									header.ID |= FIN;
									memcpy(&sendbuf[0], &header, HEADER_LEN);
									sendto(socket_server, sendbuf, HEADER_LEN, 0, (SOCKADDR *)&client_addr, sizeof(SOCKADDR));

									goto LABEL_end;
									// }
								}
								else
								{
									file.write(&recvbuf[10], DATA_LEN); //
									memcpy(&data_len, &recvbuf[Length_bit], 2);
									trans_bytes += data_len;
									memset(&recvbuf[10], 0, DATA_LEN); //及时清空缓存区
								/****************************随机丢弃ACK包！！！！***************************************************/
									makePackage();
									if(rand()%RAND_MOD_NUM!=0)
									{
										sendto(socket_server, sendbuf, HEADER_LEN, 0, (SOCKADDR *)&client_addr, sizeof(SOCKADDR));
									}
									else
									{
										cout<<"丢弃ACK包！！！！！！！！！！！"<<endl;
									}
									
									continue;
								}

							}
							else
							{
								/*******************************收到重复的包！！！**********************************************************/
								//本该收包1（然后回复acknum==1），结果收到包0，
								//由于刚刚收到的包0，此时header.acknum==0，正常调用makePackage会使得header.acknum==1
								//但是这时候还要回复acknum==0，函数细节-->memcpy(&header.ACKnum, &recvbuf[SEQ_bit], 2);
								//还是直接用makePackage即可，因为是根据收到的recvbuf[SEQ_bit]确定acknum，
								//不像发送端的SEQ是每次调用makePackage会01交替
								cout<<"接收到重传的数据包，丢弃，发送ACK！！！"<<endl;
								makePackage();
								sendto(socket_server, sendbuf, HEADER_LEN, 0, (SOCKADDR *)&client_addr, sizeof(SOCKADDR));
								memset(&recvbuf[10], 0, DATA_LEN); //及时清空缓存区
								continue;
							}
							
							
						}
						else
						{
							cout << "校验和错误！" << endl;
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
	cout << "\n**************完成一次测试！！！**************\n" << endl;
	goto LABEL_start;//
	system("pause");
}