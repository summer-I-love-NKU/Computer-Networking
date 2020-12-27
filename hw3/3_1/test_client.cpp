#include"./common.h"
int send_Ret,recv_Ret;//字节数返回值
char sendbuf[BUF_LEN]; char recvbuf[HEADER_LEN];
//sendbuf： 低十位是头部，后面是数据
//recvbuf： 只有十位，接收头部信息即可，（对于发送端）
HEADER header;
//当前连接状态
bool status=false;
clock_t _start,_end;
//进行打包
void makePackage(int length)
{
	if (!status)//刚刚开始发送
	{
		header.ID = SYN;
		header.Length = 0;
		header.SEQ = 0;//time(0) % 65535;
		header.CheckSum = 0;
		memcpy(&sendbuf, &header, HEADER_LEN);
	}
	else
	{
		header.Length = length;
		// cout<<"header.length"<<header.Length<<endl;//4086可以直接赋值
		header.SEQ =(1+header.SEQ)%2;//0 1 0 1 
		header.CheckSum = 0;
		memcpy(&sendbuf, &header, HEADER_LEN);
	}
}

//利用数据报中的所有数据(头部+数据部分)计算校验和
void doCheckSum(int length)
{
	//先将char型发送缓冲区转换成为16位的unsigned short型数据
    //sizeof： char：1   unsigned short：2  两倍关系
    /**
     * 0 1 2 3 4 5 6 7 8 9   sendbuf
     * 0   1   2   3   4     array   16位
     * */


	unsigned short* array;
	unsigned int sum = 0;
	memcpy(&length, &sendbuf[8], 2);
	int count = (length + 10) / 2;
	// cout<<length<<endl;
	
	if ((length + 10) % 2 == 0)
	{
		array = new unsigned short[count];
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &sendbuf[i * 2], 2);
		}
	}
	else
	{
		array = new unsigned short[count + 1];
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &sendbuf[i * 2], 2);
		}
		array[count] = (unsigned short)sendbuf[count * 2];
	}

	int i = 0;
	while (count--)
	{
		sum += array[i++];
		sum = (sum >> 16) + (sum & 0xFFFF);
	}
	//求出校验和后对header.CheckSum赋值,并填充到sendbuf的头部
	header.CheckSum=~sum;
	// cout<<"校验和："<<header.CheckSum<<endl;
	// memcpy(&header.CheckSum,&checkSum,2);
	memcpy(&sendbuf[0],&header, 10);
}

//检查ACKnum和SEQ是否一致
bool CheckSEQ()
{
	short seq;
	memcpy(&seq, &recvbuf[ACKnum_bit], 2);//ACKnum字段
	if (seq == header.SEQ) //这里不要按位与了因为SEQ会为0！！！！！
    {
		return true;
	}
	else
	{
		return false;
	}
}

//检查ACK
bool IsACK()
{
	short ack;
	memcpy(&ack, &recvbuf[ID_bit], 1);//1 or 2都可
	if ((ack & ACK) != 0)
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
	if ((fin & FIN) != 0) {
		return true;
	}
	else
	{
		return false;
	}
}

int main()
{
LABEL_start:
status=false;
memset(&recvbuf, 0, HEADER_LEN); 
memset(&sendbuf, 0, BUF_LEN); 
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

srand( (unsigned)time( NULL ) );//srand()函数产生一个以当前时间开始的随机种子 

//建立连接LABEL_while_1:
while (1) 
{
	/*************************************************准备建立连接*************************************/
	//制作只含头部的空包，建立SYN连接
	makePackage(HEADER_LEN);
	//发送给接收端的消息
	sendto(socket_client,sendbuf, HEADER_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));

	//接收端发来的消息，返回值是消息的字节长度
	recv_Ret = recvfrom(socket_client, recvbuf, HEADER_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
	if (recv_Ret <= 0)
	{
		//WSAGetLastError() == 10060这个参数！！！！
		cout << "超时，未接收到ACK，重传数据包！！！" << endl;
		continue;
	}
	/*********************************************接收ACK成功，连接建立*************************************/
	else 
	{
		if (IsACK() && CheckSEQ())
		{
			status = true;
			cout << "序列号匹配正确！" << endl << "客户端（发送）与服务端（接收）建立连接成功！"<<endl;
			LABEL_input_filename:
			cout << "请输入要传输的文件序号（1 2 3 4 5）：";
			int fileID;cin >> fileID;
			if(fileID!=0&&fileID!=1&&fileID!=2&&fileID!=3&&fileID!=4&&fileID!=5)
			{
				cout<<"输入错误！！！"<<endl;
				goto LABEL_input_filename;
			}
			//-----------------------告知文件名：-------------------------
			header.Length=fileID;//这样借用一下字段
			memcpy(&sendbuf[0], &header, 10);//
			sendto(socket_client, sendbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
			// memcpy(sendbuf,filename_list[fileID].c_str(),filename_list[fileID].length()+1);
			// // cout<<sendbuf<<endl;
			// sendto(socket_client, sendbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
			//--------------------------------------------------
			fstream file;
			char* __root;
			if((__root = getcwd(NULL, 0)) == NULL)
			{
				perror("getcwd error");
			}
			string _root=string(__root)+"\\test_file\\";
			file.open(_root+filename_list[fileID], ios::in | ios::binary);
			if (!file)
			{
				cout << "failed to open this file!!!" << endl;
				goto LABEL_input_filename;
			}
			else 
			{

			/***************************************文件读取成功，准备传输，在while循环里*********************************************************/
//传输文件LABEL_while_2:
				int trans_num = 0,trans_bytes=0;
				_start=clock();
				while (1)
				{
					trans_num++;
					file.read(&sendbuf[10], DATA_LEN);
					if (file.gcount() < DATA_LEN)
					{
						//读取结束了
						header.ID |= FIN;//或操作使得FIN位置为1
						memcpy(&sendbuf[0], &header, 10);//
					}
					// cout<<file.tellg()<<endl;
					//制作头部
					makePackage(file.gcount());
					//设置校验和并填充
					doCheckSum(file.gcount());
		/*****************************随机丢弃发送的数据包******************************************************************************/
					//发送数据包,随机丢弃（不发送）
					if(rand()%RAND_MOD_NUM!=0)//10%~15%的概率丢弃，即遇到rand()%8==0
						sendto(socket_client, sendbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
					else
						cout<<"丢弃数据包！！！！！！！！！！！"<<endl;
					
					
					//接收ACK
					recv_Ret = recvfrom(socket_client, recvbuf, HEADER_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
					// cout<<recv_Ret<<endl;//10
					if (recv_Ret < 0)
					{
						//(WSAGetLastError() == 10060)
						cout << "超时，未接收到ACK，重传数据包！！！" << endl;
						/*处理：
						1.读文件指针回退，
						2.trans_num--
						3.makepackage序列号要注意，之前不注意0或1是因为接收端不检查，接收端没有做丢弃acknum的测试，
						接收端做丢弃acknum，就要处理来自发送端的重复的包，就必须检查序列号，发送端就要做好0 1交替准确
						也很简单，调用两次makePackage即可！！！
						刚刚发了包0，多调用一次makePackage（SEQ==1）再发包0，makePackage（SEQ==0）就可以了
						*/
						file.seekg(-file.gcount(),ios::cur);//向前移动
						trans_num--;
						makePackage(0);//重传的处理，多调用一次使得序列号和之前一样
						continue;
					}
					else 
					{
						if (IsACK())
						{
							if (CheckSEQ())
							{
								if(IsFIN())
								{
									cout << "第" << trans_num << "次传输成功！传输" << file.gcount()<<"字节" << endl;
									cout << "文件"<<filename_list[fileID]<<"传输完成！" << endl;
									trans_bytes+=file.gcount();

									_end=clock();
									double endtime=(double)(_end-_start)/CLOCKS_PER_SEC;
									cout<<"\n传输用时:"<<endtime<<"秒"<<endl;
									cout<<"传输数据量："<<trans_bytes<<"字节 ( "<<trans_bytes/1024<<"KB )"<<endl;

									file.close();
									goto LABEL_end;
								}
								else
								{
									cout << "第" << trans_num << "次传输成功！" <<endl<< "传输" << file.gcount()<<"字节" << endl;
									// cout<<"序列号："<<header.SEQ<<endl;
									
									trans_bytes+=file.gcount();
									memset(&sendbuf[10], 0, DATA_LEN);//注意清空sendbuf！！！！！！！保险
									continue;
								}
								
							}
							else
							{
								cout<<"序列号不对？？？应该不会发生这种情况！！！"<<endl;
							}
						}
						else
						{
							cout<<"不是ACK？？？应该不会发生这种情况！！！"<<endl;
						}
					}
				}
			}
		}
		
	}
}


LABEL_end:
	//关闭socket连接//清理
	closesocket(socket_client);WSACleanup();
	cout << "\n**************完成一次测试！！！**************\n" << endl;
	goto LABEL_start;//
	system("pause");
}