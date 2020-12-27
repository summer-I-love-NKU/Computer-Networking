#include"./common.h"
int send_Ret,recv_Ret;//bytes数返回值
PACKAGE recvbuf,sendbuf;//结构体也能发送！！！ (char*)&sendbuf!!!!
int fileID;
int expected_ack,trans_bytes;
int T_a=0,T_b=0,dup_trans_num=0;//处理接收端断开,自己还不知道
int cur_ack=0,dup_ack_num=0;//处理快速重传
/**************************************************************/
int SSTHRESH=16;//慢启动门限 初值为16
int CWND=1;//拥塞窗口，初值为1  
int avoid_num=0;
//三种状态
#define SLOW 1
#define AVOID 2
#define QUICK 3
int state=1;
/*******************************************************************/
void makePackage(int length);
void doCheckSum(int length);
bool IsACK();
bool IsFIN();
bool Check_expected_ACK();
/*******************************************************************/
int main()
{
LABEL_start:
Sleep(1000);status=false;trans_bytes=0;bool read_flag=1;
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
// 设置发送超时//设置接收超时
setsockopt(socket_client,SOL_SOCKET,SO_SNDTIMEO, (char *)&nNetTimeout,sizeof(int));
setsockopt(socket_client,SOL_SOCKET,SO_RCVTIMEO, (char *)&nNetTimeout,sizeof(int));
srand( (unsigned)time( NULL ) );
while (1) 
{
	/*************************************************准备建立连接*************************************/
	makePackage(BUF_LEN);
	CLIENT_SEND;
	recv_Ret = recvfrom(socket_client, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
	if (recv_Ret <= 0)
	{//WSAGetLastError() == 10060
		cout << "timeout,connect again!" << endl;//cout << "超时，未接收到ACK，重新尝试连接！！！" << endl;
	}
	/*********************************************接收ACK成功，连接建立*************************************/
	else 
	{
		if (IsACK())//代表着对方给了回复（回复一定是ACK的，检查ACK就能判断是否连接）
		{
#if 1
			status = true;expected_ack=2;
			//之后数据传输，发送序列号为从1开始，期望的ACKnum为2

			cout << " connect!"<<endl;//cout << " 客户端（发送）与服务端（接收）建立连接成功！"<<endl;
LABEL_input_filename:
			cout << "请输入要传输的文件序号（1 2 3 4 5）：";
			cin >> fileID;cout<<fileID<<endl;
			if(fileID!=0&&fileID!=1&&fileID!=2&&fileID!=3&&fileID!=4&&fileID!=5)
			{
				cout<<"输入错误！！！"<<endl;
				goto LABEL_input_filename;
			}
			//-----------------------告知文件名(不占用序列号，没有调用makepackage函数)：-------------------------
			memcpy(&sendbuf.data, filename_list[fileID].c_str()+'\0', filename_list[fileID].length()+1);//
			CLIENT_SEND;
			//-----------------确认对方接收到文件名
			do
			{
				recv_Ret = recvfrom(socket_client, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR *)&server_addr, &recv_para_len);
			} while (recv_Ret < 0);
			if (IsACK())
			{
				cout<<"***************start transport****************"<<endl;//cout<<"对方接收到文件名，开始传输！！！"<<endl;
			}
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
				int _s1,_s2,_file_size;
				file.seekg(0,ios::end);_s2=file.tellg();
				file.seekg(0,ios::beg);_s1=file.tellg();
				_file_size=_s2-_s1;
#endif
			/***************************************文件读取成功，准备传输，在while循环里*********************************************************/
//传输文件LABEL_while_2:
				
				_start=clock();
				p_Base=1;p_NextSeqNum=1;
//LABEL_GBN_START:
				while (1)
				{	

					cout<<"~~~~~~~~~~~~CWND:"<<CWND<<"~~~~~~~~~~~~~~~"<<endl;			
CC_AVOID:
					if(CWND>=SSTHRESH)
					{
						cout<<"###############################进入拥塞避免 CWND:"<<CWND<<" ###############################"<<endl;
						state=AVOID;
					}


LABEL_GBN_SEND:
					//窗口滑动，发送多个分组
					while((p_NextSeqNum<p_Base+CWND)&&read_flag)//这里是while！！！不是if！！！！
					{
						if(p_NextSeqNum==SEQ_SIZE)
						{//实现循环，这样也可以很简单
							p_Base-=p_Base;p_NextSeqNum-=p_Base;
						}

						file.read((char*)&sendbuf.data, DATA_LEN);//?????????file.read(sendbuf.data, DATA_LEN);
						if (file.gcount() < DATA_LEN)
						{
							//读取结束了
							sendbuf.ID |= FIN;//或操作使得FIN位置为1
							read_flag=0;//不再读取了
						}
						makePackage(file.gcount());
						//设置校验和并填充
						doCheckSum(file.gcount());
						//加入发送缓冲队列
						PACKAGE t1=sendbuf;
						Send_Buf_Queue.push(t1);

						if(RANDOM_EXPR)
						{
							CLIENT_SEND;
							cout<<"send seq: "<<sendbuf.SEQ<<endl;//cout<<"发送第 "<<sendbuf.SEQ<<" 个数据包!!!"<<endl;
						}
						else
						{
							cout<<"*********随机丢弃数据包 "<<sendbuf.SEQ<<"*********"<<endl;
						}
						
						p_NextSeqNum++;
						
					}
LABEL_GBN_RECV:				
					recv_Ret = recvfrom(socket_client, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
CC_SLOW:		
					if (recv_Ret < 0)
					{
						state=SLOW;
						SSTHRESH/=2;
						CWND=1;
						cout<<"###############################进入慢启动 CWND: "<<CWND<<" ###############################"<<endl;
						
#if 1
						if(p_Base==T_a&&p_NextSeqNum==T_b)
						{
							dup_trans_num++;
						}
						else
						{
							dup_trans_num=0;
							T_a=p_Base;T_b=p_NextSeqNum;
						}
						if(dup_trans_num==3)
						{
							cout<<"对方无应答，应该是退出了，结束！！"<<endl;
							goto LABEL_Complete;
						}
						

						/**************************************/
						/*
						会出现最后收不到FIN的情况，也就只能重传，这时候检查一下是否base和nextnum相等，相等就结束了
						*/
						if((p_Base==p_NextSeqNum&&p_Base!=1))//||read_flag==0)
						{
							cout<<"我认为已经结束了！！！"<<endl;//<<read_flag<<endl;
							goto LABEL_Complete;
						}					
#endif		
						/*******************超时，重传这些包*****************************
						 * base--next-1
						 * 0 1 2 3 4 5 6 7 8
						 *   |       | 已发送1 2 3 4 5
						 * 收到ack2，收不到ack3-->1.对方未收到seq3，期待收到seq3 2.对方收到了seq3，但是ack3丢失
						 * 都执行重传，重传1 2 3 4 5（根据队列很方便）
						 * */
LABEL_TRANS_AGAIN:
						//----------------
						for (int i = p_Base; i < p_NextSeqNum; i++)//-------------
						{
							sendbuf=Send_Buf_Queue.front();
							if(1)//RANDOM_EXPR)
							{
								CLIENT_SEND;
							}
							else
							{
								cout<<"*********************随机丢弃重传的数据包！！！！！******************"<<endl;
							}
							PACKAGE t1=Send_Buf_Queue.front();
							Send_Buf_Queue.push(t1);
							Send_Buf_Queue.pop();
						}
						cout << "超时未接收到ACK，重传 "<<p_NextSeqNum<<"-"<<p_Base<<" == "<<p_NextSeqNum-p_Base<<"个数据包！！！" << endl;
						
						goto LABEL_GBN_RECV;
					}
					else 
					{
LABEL_GBN_GETACK:
						if(recvbuf.ACKnum==expected_ack||IsFIN())// ==!!!!!
						{
							expected_ack=recvbuf.ACKnum+1;
							if(IsFIN())
							{
								trans_bytes+=recvbuf.Length;
								cout << "successfully trans " << recvbuf.ACKnum-1 << endl;
								// cout << "第" << recvbuf.ACKnum-1 << "次传输成功！传输" << recvbuf.Length<<"bytes" << endl;
LABEL_Complete:
								cout << "文件"<<filename_list[fileID]<<"("<<_file_size<<" bytes)传输完成！" << endl;

								_end=clock();
								double trans_time=(double)(_end-_start)/CLOCKS_PER_SEC;
								//cout<<"\n窗口大小"<<CWND<<endl;
								cout<<"\n传输用时:"<<trans_time<<"秒"<<endl;
								//cout<<"传输数据量(不准确)："<<trans_bytes<<" bytes ( "<<trans_bytes/1024<<"KB )"<<endl;
								cout<<"传输数据量(不包含重复数据)："<<_file_size<<" bytes ( "<<_file_size/1024<<"KB )"<<endl;
								cout<<"吞吐率："<<_file_size/trans_time<<" bytes/s ( "<<_file_size/1024/trans_time<<"KB/s )"<<endl;

								file.close();
								//接收到对方的FIN，发送ACK(再次的FIN)！！！退出即可
								sendbuf.ID |= ACK;
								if(1)//RANDOM_EXPR)//即遇到rand()%RAND_MOD_NUM==0丢弃
								{
									CLIENT_SEND;
								}
								else
								{
									CLIENT_SEND;
									cout<<"************************随机丢弃挥手的ACK数据包！！！！！*******************"<<endl;
								}

								goto LABEL_end;
							}
							else
							{
								cout << "successfully trans " << recvbuf.ACKnum-1 <<"   CWND+=1 !!!"<< endl;
								//cout << "第" << recvbuf.ACKnum-1 << "次传输成功！" << "传输" << recvbuf.Length<<" bytes" << endl;
								trans_bytes+=recvbuf.Length;
CONTROL_1:								
								/***************收到一个ACK  滑动窗口吧,注意缓存队列要出队一个！！！不然影响重传******* */
								p_Base=recvbuf.ACKnum;
								Send_Buf_Queue.pop();
								//-------------------------------------------
								if(state==SLOW)
								{
									CWND*=2;
									// CWND+=1;
								}
								else if(state==AVOID)
								{
									// avoid_num++;
									// if(avoid_num==CWND)
										CWND+=1;
								}
								else
								{
									state=AVOID;
									CWND=SSTHRESH;
									cout<<"############################进入拥塞避免 CWND: "<<CWND<<" ###############################"<<endl;
									
								}
								//---------------------------------------------


							}
							
						}
						else if(recvbuf.ACKnum<expected_ack)
						{
							if(recvbuf.ACKnum==cur_ack)
							{
								dup_ack_num++;
								cout << "收到之前的ACKnum且重复！！！: " << recvbuf.ACKnum << ""<< endl;
							}
							else
							{
								dup_ack_num=0;/*不执行动作，等待收不到ACK，超时重传给对方*/
								cout << "收到之前的ACKnum: " << recvbuf.ACKnum << "！不执行动作即可"<< endl;
								cur_ack=recvbuf.ACKnum;
							}
CC_QUICK:
							if(dup_ack_num==2)
							{
								//快速恢复 RENO算法
								dup_ack_num=0;

								state=QUICK;
								SSTHRESH=CWND/2;
								CWND=SSTHRESH+3;
								cout<<"############################进入快速恢复 CWND: "<<CWND<<" ###############################"<<endl;
								
								goto LABEL_TRANS_AGAIN;
								
							}

						}
						else if(recvbuf.ACKnum>expected_ack)
						{
							cout<<"----------收到超前的ACKnum,可以加速滑动！！！ ACKnum: "<<recvbuf.ACKnum<<"cwnd+=n!!!"<<endl;
							p_Base=recvbuf.ACKnum;
							//p_Base多移动了
							for (int i = 0; i < recvbuf.ACKnum-expected_ack+1; i++)
							{
								Send_Buf_Queue.pop();
							}
							expected_ack=recvbuf.ACKnum+1;//!!!!!
							trans_bytes+=(recvbuf.ACKnum-expected_ack+1)*DATA_LEN;//
CONTROL_2:
							//-------------------------------------------------------
							if(state==SLOW)
							{
								//CWND+=recvbuf.ACKnum-expected_ack+1;
								CWND*=2;
							}
							else if(state==AVOID)
							{
								// avoid_num+=recvbuf.ACKnum-expected_ack+1;
								// if(avoid_num==CWND)
									CWND+=1;
							}
							else
							{
								state=AVOID;
								CWND=SSTHRESH;
								cout<<"############################进入拥塞避免 CWND: "<<CWND<<" ###############################"<<endl;
								
							}
							//-------------------------------------------------------

						}
						else
						{
							cout<<"error,不可能发生！"<<endl;return 0;
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
	cout << "\n**************完成一次测试！！！**************\n\n\n" << endl;
	//goto LABEL_start;//
LABEL_exit:
	//system("pause");
	return 0;
}

//进行打包(只是制作头部)
void makePackage(int length)
{
	if (!status)//刚刚开始发送
	{
		sendbuf.ID = SYN;
		sendbuf.Length = 0;
		sendbuf.SEQ = 0;
		sendbuf.CheckSum = 0;
	}
	else
	{
		sendbuf.Length = length;
		sendbuf.SEQ =(sendbuf.SEQ+1)%65536;
		sendbuf.CheckSum = 0;
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
	int count = length / 2+bool(length%2);
	
	ushort* array = new ushort[count+HEADER_LEN/2];
	if (length % 2 == 0)//count==5
	{
		for (int i = 0; i < count; i++)
		{
			memcpy(&array[i], &sendbuf.data[i * 2], 2);
		}
	}
	else
	{
		for (int i = 0; i < count-1; i++)//count==6: 012345678910 
		{
			memcpy(&array[i], &sendbuf.data[i * 2], 2);
		}
		array[count-1] = (ushort)sendbuf.data[length-1];
	}

	//头部部分
	array[count]=sendbuf.SEQ;
	array[count+1]=sendbuf.ACKnum;
	array[count+2]=sendbuf.CheckSum;
	array[count+3]=sendbuf.ID;
	array[count+4]=sendbuf.Length;

	count+=HEADER_LEN/2;uint sum=0;
	while (count--)
	{
		sum += array[count];
		// cout<<array[count]<<endl;
		sum = (sum >> 16) + (sum & 0xFFFF);
	}
	
	sendbuf.CheckSum=~sum;
	// cout<<"校验和："<<sendbuf.CheckSum<<endl;

}



//检查ACK,这里不能直接用等于，因为ID有三个值，0x0001 0x0010 0x0100
//与0x1 0x2 0x4按位与不等于0，说明就等于这个标志位
bool IsACK()
{
	if ((recvbuf.ID & ACK) != 0)
		return true;
	else
		return false;
}

//判断是不是最后一个包的ACK
bool IsFIN()
{
	if ((recvbuf.ID & FIN) != 0) 
		return true;
	else
		return false;
}

bool Check_expected_ACK()
{
	return recvbuf.ACKnum<=sendbuf.SEQ+1;
}

