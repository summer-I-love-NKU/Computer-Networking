#include"./common.h"
int send_Ret,recv_Ret;//bytes数返回值
PACKAGE recvbuf,sendbuf;//结构体也能发送！！！ (char*)&sendbuf!!!!
int fileID;
int expected_ack,trans_bytes;
int T_a=0,T_b=0,dup_trans_num=0;//处理接收端断开,自己还不知道
int cur_ack=0,dup_ack_num=0;//处理快速重传
int WINDOW_WIDTH;
/*******************************************************************/
void makePackage(int length);
void doCheckSum(int length);
bool IsACK();
bool IsFIN();
bool Check_expected_ACK();
// int GetACKnum(){return recvbuf.ACKnum;}
/*******************************************************************/
int main()//int argc,char* argv[]) //命令行执行传参方法！！！
{
cin>>WINDOW_WIDTH;//WINDOW_WIDTH=atoi(argv[1]);
//fileID=atoi(argv[2]);
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
//srand()函数产生一个以当前时间开始的随机种子 
srand( (unsigned)time( NULL ) );

//建立连接LABEL_while_1:
while (1) 
{
	/*************************************************准备建立连接*************************************/
	//制作只含头部的空包，建立SYN连接
	makePackage(BUF_LEN);
	//发送给接收端的消息
	CLIENT_SEND;

	//接收端发来的消息，返回值是消息的bytes长度
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
			/***************************************文件读取成功，准备传输，在while循环里*********************************************************/
//传输文件LABEL_while_2:
				
				_start=clock();
				
				p_Base=1;p_NextSeqNum=1;

				while (1)
				{
					// if(p_NextSeqNum==SEQ_SIZE)
					// {//实现循环，这样也可以很简单
					// 	p_Base-=p_Base;p_NextSeqNum-=p_Base;
					// }
					
LABEL_GBN_1:
					/*****************读取、发送*****************
					 * 0 1 2 3 4 5 6 7 8 9
					 *   |         |    n=5(1 2 3 4 5)
					 *   base     next
					 * 
					 * 发送端何时判断结束！！！：
					 * 自己读完就不读了，窗口不滑动了
					 * 
					 * 最后一次base和nextnum相差一，这时候可以直接退出？？！！！
					 * */
					while((p_NextSeqNum<p_Base+WINDOW_WIDTH)&&read_flag)//这里是while！！！不是if！！！！
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
						// cout<<"队列长度："<<Send_Buf_Queue.size()<<endl;
						// cout<<"队头元素的数据："<<Send_Buf_Queue.front().data<<endl;
// LABEL_debug:
						// cout<<"校验和！！！："<<doCheckSum1(sendbuf)<<endl;//-------------正确！！！！
						// cout<<"校验和！！！："<<doCheckSum1(Send_Buf_Queue.front())<<endl;

			/*****************************随机丢弃发送的数据包******************************************************************************/
						//发送数据包,随机丢弃（不发送）
						if(RANDOM_EXPR)//即遇到rand()%RAND_MOD_NUM==0丢弃
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
LABEL_GBN_2:				
					//*****************************接收ACK********************
					recv_Ret = recvfrom(socket_client, (char*)&recvbuf, BUF_LEN, 0, (SOCKADDR*)&server_addr, &recv_para_len);
		
					//-加入时延，就出了问题------------对方过早退出，自己还没收到
					if (recv_Ret < 0)
					{
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
						
LABEL_comment1:
						/**************************************/
						/*
						会出现最后收不到FIN的情况，也就只能重传，这时候检查一下是否base和nextnum相等，相等就结束了
						*/
						if((p_Base==p_NextSeqNum&&p_Base!=1))//||read_flag==0)
						{
							//直接根据readflag判断,不行！！！！，
							cout<<"我认为已经结束了！！！"<<endl;//<<read_flag<<endl;
							goto LABEL_Complete;
						}					
		
						/*******************超时，重传这些包*****************************
						 * base--next-1
						 * 0 1 2 3 4 5 6 7 8
						 *   |       | 已发送1 2 3 4 5
						 * 收到ack2，收不到ack3-->1.对方未收到seq3，期待收到seq3 2.对方收到了seq3，但是ack3丢失
						 * 都执行重传，重传1 2 3 4 5（根据队列很方便）
						 * 但是接收方处理时要能处理比期望seq小的seq号！！！！
						 * 
						 * 
						 * 队列遍历，只能pop push
						 * */
						// cout<<"队列长度："<<Send_Buf_Queue.size()<<endl;
						// int len=Send_Buf_Queue.size();
LABEL_TRANS_AGAIN:
						//----------------
						for (int i = p_Base; i < p_NextSeqNum; i++)//-------------
						{
							sendbuf=Send_Buf_Queue.front();
							if(1)//即遇到rand()%RAND_MOD_NUM==0丢弃
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
							// cout<<"队头元素的数据："<<Send_Buf_Queue.front().data<<endl;
						}
						cout << "超时未接收到ACK，重传 "<<p_NextSeqNum<<"-"<<p_Base<<" == "<<p_NextSeqNum-p_Base<<"个数据包！！！" << endl;
						
						goto LABEL_GBN_2;
					}
					else 
					{
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
								cout<<"\n窗口大小"<<WINDOW_WIDTH<<endl;
								cout<<"传输用时:"<<trans_time<<"秒"<<endl;
								//cout<<"传输数据量(不准确)："<<trans_bytes<<" bytes ( "<<trans_bytes/1024<<"KB )"<<endl;
								cout<<"传输数据量(不包含重复数据)："<<_file_size<<" bytes ( "<<_file_size/1024<<"KB )"<<endl;
								cout<<"吞吐率："<<_file_size/trans_time<<" bytes/s ( "<<_file_size/1024/trans_time<<"KB/s )"<<endl;

								file.close();
								//接收到对方的FIN，发送ACK(再次的FIN)！！！退出即可
								sendbuf.ID |= ACK;
								if(1)//即遇到rand()%RAND_MOD_NUM==0丢弃
								{
									CLIENT_SEND;
								}
								else
								{
									cout<<"************************随机丢弃挥手的ACK数据包！！！！！*******************"<<endl;
								}

								goto LABEL_end;
							}
							else
							{
								cout << "successfully trans " << recvbuf.ACKnum-1 << endl;
								//cout << "第" << recvbuf.ACKnum-1 << "次传输成功！" << "传输" << recvbuf.Length<<" bytes" << endl;
								trans_bytes+=recvbuf.Length;
LABEL_GBN_3:								
								/***************收到一个ACK  滑动窗口吧,注意缓存队列要出队一个！！！不然影响重传*****************************
								 * --------------------------------------
								 * 比如发送0 1 2 3 ，收到ack2(seq1发送成功)，可以直接忽略ack1 ，
								 * TCP就是这么用，因为接收端是累计确认的
								 * 所以p_Base=GetACKnum();更好
								 * */
								p_Base=recvbuf.ACKnum;
								// cout<<"队头元素"<<Send_Buf_Queue.front().data<<"队列长度"<<Send_Buf_Queue.size()<<endl;
								Send_Buf_Queue.pop();
								// cout<<"队头元素"<<Send_Buf_Queue.front().data<<"队列长度"<<Send_Buf_Queue.size()<<endl;
								//-------------------------------------------
								
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
								dup_ack_num=0;
								cout << "收到之前的ACKnum: " << recvbuf.ACKnum << "！不执行动作即可"<< endl;
								cur_ack=recvbuf.ACKnum;
							}
							if(dup_ack_num==2)
							{
								//快速重传
								dup_ack_num=0;
								cout<<"************************执行快速重传！！！*******************"<<endl;
								goto LABEL_TRANS_AGAIN;
								
							}
							/*不执行动作，等待收不到ACK，超时重传给对方*/

							/*
							1 2 3 4 5 6 7 8 9 
							假如seq7丢失，也只能等到接收到1 2 3 4 5 6的ACK才知道（ack7来不了）
							实际上不也应该是这样吗，7的ACK肯定比4 5 6 都晚呀（如果你用定时器）
							
							但编程很快，你也可以想办法，收到4的时候不发送seq8（你通过什么方法检测出来seq7发送失败）
							如果采用现有方法是无法检测的
							使用线程，收到ack就滑动窗口，不然就？？

							啊是我的窗口太小，应该模拟窗口大，延时长的情况，
							所以我的没有定时器的方案，就是GBN的思想吧？？？！！！！

							*/
						}
						else//recvbuf.ACKnum>expected_ack
						{
LABEL_comment2:
							/*recvbuf.ACKnum>expected_ack
							比如1 2 3 4 5 6 7 8
							收到了ack2（seq1发送成功，发送seq6（N=5）） ack3 
							期望收到ack4 
							结果收到ack6（说明seq5发送成功只是自己没收到，但自己已经不需要ack6之前的了）
							显然ack6>expected_ack==ack4 注意凡是发过来的acknum都是合理的
								接收方的累计确认保证了这一点！！！！————CheckSEQ_EXPECTED 和 makepackage的ACKnum=SEQ+1
							那么只需要
								1.移动base即可！！！
								2.pop需要多几次
								3.！！！修改expected_ack
								4.trans_bytes修改


							1 2 3 4 5 expect=2
							  2 3 4 5 6 expect=3
							    3 4 5 6 7 expect=4
						acknum=7   7 8 9 10 11 base从3移动到7！！！

							--这种情况是UDP的不稳定吗？？？一方发送多条信息，另一方会胡乱接收乱序的？
							*/

							cout<<"----------收到超前的ACKnum,可以加速滑动，开心！！！ ACKnum: "<<recvbuf.ACKnum<<endl;
							p_Base=recvbuf.ACKnum;
							//p_Base多移动了
							for (int i = 0; i < recvbuf.ACKnum-expected_ack+1; i++)
							{
								Send_Buf_Queue.pop();
							}
							expected_ack=recvbuf.ACKnum+1;//!!!!!
							trans_bytes+=(recvbuf.ACKnum-expected_ack+1)*DATA_LEN;//

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

