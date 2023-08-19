/*
* 本程序为S4代理服务程序,运行于windows本地，实现系统代理（windows原生只支持不加密的S4）
* 通过本程序实现S5-client去连接正正的远程S5代理服务器，亦可实现自定义的加密方式
*/
#include"Winsock2.h"
#pragma comment(lib, "ws2_32")
#include<windows.h>
#include<iostream>
#include <string>
#include <ws2tcpip.h>
#include "SafeModule.h"
using namespace std;



/// 宏定义
#define PORT 5039
#define DATA_BUFSIZE 4096




//#define OutErr(a) cout << (a) <<",errCode="<< WSAGetLastError() << endl 
//#define OutMsg(a) cout << (a) << endl;

#define OutErr(a)
#define OutMsg(a)

#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )

void LOG_BUF(string msg, uint8_t* data, int len)
{
	//int i = 0;
	//cout << msg;
	//for (i = 0;i < len;i++)
	//{
	//	printf("%02X ", data[i]);
	//}
	//cout << endl;
}


CHAR S5_SERVICE_IP[32];//	"121.5.149.46"
USHORT S5_SERVICE_PORT = 5038;
BYTE enable_s5 = 1;//0,1
BYTE s5_method = 0x81;//0,0x81
//
void ParseArgc(int argc,char *argv[])
{
	::strcpy_s(S5_SERVICE_IP, "121.5.149.46");
	S5_SERVICE_PORT = 5038;

	for (int i = 1;i < argc;i++)
	{
		if (i == 1)
		{
			::strcpy_s(S5_SERVICE_IP, argv[1]);
		}
		else if (i == 2)
		{
			S5_SERVICE_PORT = atoi(argv[2]);
		}
	}

	cout << S5_SERVICE_IP << ":" << S5_SERVICE_PORT << endl;
}

BYTE checkSum(BYTE *p,int len)
{
	BYTE a = 0;
	while (len--)
		a += *p++;

	return a;
}

void randKey(BYTE *kk,int len)
{
	srand((int)time(0));
	for (int i = 0;i < len;i++)
	{
		kk[i] = rand() & 0xff;
	}
}

int readSocketData(SOCKET client,UCHAR *buf,int len)
{
	int r_len = recv(client, (CHAR*)buf, len, 0);
	if (r_len < 0)
	{
		OutErr("recv");
		closesocket(client);
	}
	else if (r_len == 0)
	{
		//cout << "connection closed..." << endl;
		int err = shutdown(client, SD_SEND);
		if (err == SOCKET_ERROR)
		{
			OutErr("shutdown");
		}
		
	}
	return r_len;
}

int sendSocketData(SOCKET client, UCHAR* buf,int len)
{
	int w_len = send(client, (CHAR*)buf, len, 0);
	if (w_len == SOCKET_ERROR)
	{
		OutErr("send");
		closesocket(client);	
	}
	return w_len;
}

SOCKET connect_byS5(SafeModule& sm,BYTE ip[4], USHORT port)
{
	BYTE w_buf[32];
	BYTE r_buf[32];
	BYTE key_tmp[16] = {0};
	int w_len=0, r_len=0;
	SOCKET s5 = socket(AF_INET, SOCK_STREAM, 0);
	if (s5 < 0)
	{
		OutErr("socket");
		return -1;
	}
	struct sockaddr_in ipaddr;
	memset(&ipaddr, 0, sizeof(struct sockaddr_in));
	ipaddr.sin_family = AF_INET;
	inet_pton(AF_INET, S5_SERVICE_IP, &ipaddr.sin_addr.s_addr);
	ipaddr.sin_port = htons(S5_SERVICE_PORT);
	int ret = connect(s5, (struct sockaddr*)&ipaddr, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		OutErr("connect s5");
		return -1;
	}

	//协商加密
	w_buf[0] = 5;
	w_buf[1] = 1;
	w_buf[2] = s5_method;
	LOG_BUF("-->S5", w_buf,3);
	w_len = sendSocketData(s5, w_buf, 3);
	if (w_len <= 0)
	{
		goto err_exit;
	}

	r_len = readSocketData(s5, r_buf,32);
	if (r_len <= 0)
	{
		goto err_exit;
	}
	LOG_BUF("<--S5", r_buf, r_len);
	if (r_buf[1] != s5_method || r_buf[0] != 5)
	{
		goto err_exit;
	}

	//
	switch (s5_method)
	{
		case 1:
		{
			//user+passwd
		}
		break;

		case 0x81:
		{

			randKey(key_tmp, 16);

			w_buf[0] = 0x05;
			w_buf[1] = 16;
			memcpy(w_buf+2,key_tmp,16);
			w_buf[18] = checkSum(w_buf, 18);
			w_len = sendSocketData(s5, w_buf, 19);
			if (w_len <= 0)
			{
				goto err_exit;
			}
			r_len = readSocketData(s5, r_buf, 32);
			if (r_len <= 0)
			{
				goto err_exit;
			}
			if (r_buf[1] != 0 || r_buf[0] != 5)
			{
				goto err_exit;
			}

			sm.setKey(key_tmp);

		}
		break;

		default:
		break;
	}

	//connect 
	w_buf[0] = 0x05;
	w_buf[1] = 1;
	w_buf[2] = 0;
	w_buf[3] = 1;
	::memcpy(w_buf + 4, ip, 4);
	w_buf[8] = port>>8;
	w_buf[9] = port&0xff;
	LOG_BUF("-->S5", w_buf, 10);
	w_len = sendSocketData(s5, w_buf, 10);
	if (w_len <= 0)
	{
		goto err_exit;
	}

	r_len = readSocketData(s5, r_buf, 32);
	if (r_len <= 0)
	{
		goto err_exit;
	}
	LOG_BUF("<--S5", r_buf, r_len);
	if (r_buf[1] != 0 || r_buf[0] != 5)
	{
		goto err_exit;
	}

	return s5;

	err_exit:
	shutdown(s5, SD_SEND);
	closesocket(s5);
	return -1;
}

BYTE s4init(SafeModule &sm,SOCKET &app_client, SOCKET &app_service)
{
	int r_len = 0;
	BYTE r_buf[16];
	CHAR ip_str[64];

	r_len = readSocketData(app_client, r_buf, 16);
	if (r_len <= 0)
	{
		return 0;
	}

	if (r_buf[0] != 4)//socks4 
	{
		return 0;
	}
	//04 01 27 11 79 05 95 2e 00
	//121.5.149.46:10001
	if (r_buf[1] == 1)
	{
		struct sockaddr_in ipaddr;
		memset(&ipaddr, 0, sizeof(struct sockaddr_in));
		ipaddr.sin_family = AF_INET;
		ipaddr.sin_addr.S_un.S_un_b.s_b1 = r_buf[4];
		ipaddr.sin_addr.S_un.S_un_b.s_b2 = r_buf[5];
		ipaddr.sin_addr.S_un.S_un_b.s_b3 = r_buf[6];
		ipaddr.sin_addr.S_un.S_un_b.s_b4 = r_buf[7];
		ipaddr.sin_port = htons(r_buf[2] << 8 | r_buf[3]);

		if (enable_s5 > 0)
		{
			app_service = connect_byS5(sm,r_buf + 4, r_buf[2] << 8 | r_buf[3]);
			if (app_service < 0)
			{
				OutErr("socket");
				return 0x5b;
			}
		}
		else
		{
			app_service = socket(AF_INET, SOCK_STREAM, 0);
			if (app_service < 0)
			{
				OutErr("socket");
				return 0x5b;
			}
			int ret = connect(app_service, (struct sockaddr*)&ipaddr, sizeof(struct sockaddr_in));
			if (ret < 0)
			{
				inet_ntop(AF_INET, &ipaddr, ip_str, sizeof(ip_str));
				string ip = ip_str;
				string port = to_string(ntohs(ipaddr.sin_port));
				string msg = "connect" + ip + ":" + port;
				OutErr(msg);
				return 0x5b;
			}
		}
	}
	else //if (r_buf[2] == 2)
	{
		return 0x5c;
	}

	return 0x5a;
}

int s4DataProcess(SafeModule& sm, SOCKET& app_client, SOCKET& app_service)
{
	FD_SET   ReadSet;
	BYTE   Buffer[DATA_BUFSIZE];
	int r_len = 0;
	int w_len = 0;
	while (1)
	{
		FD_ZERO(&ReadSet);
		FD_SET(app_client, &ReadSet);
		FD_SET(app_service, &ReadSet);

		if (select(0, &ReadSet, NULL, NULL, NULL) == SOCKET_ERROR)
		{
			break;
		}

		//客户应用有数据发过来
		if (FD_ISSET(app_client, &ReadSet))
		{
			do
			{
				r_len = readSocketData(app_client, Buffer, DATA_BUFSIZE);
				if (r_len <= 0)
				{
					return -1;
				}
				else
				{
					sm.encryp(Buffer, r_len);
					w_len = sendSocketData(app_service, Buffer, r_len);//阻塞接口,写完才返回,内核缓冲不够时,会阻塞直到有多余的缓冲
					if (w_len > 0)
					{
					}
					else
					{
						return -1;
					}
					if (r_len < DATA_BUFSIZE)
					{
						//数据读空
						break;
					}

				}			
			} while (1);		
		}
	

		//服务应用有数据发过来
		if (FD_ISSET(app_service, &ReadSet))
		{		
			do
			{
				r_len = readSocketData(app_service, Buffer, DATA_BUFSIZE);
				if (r_len <= 0)
				{
					return -1;
				}
				else
				{
					sm.decrypt(Buffer, r_len);
					w_len = sendSocketData(app_client, Buffer, r_len);//阻塞方式,写完才返回,内核缓冲不够时,会阻塞直到有多余的缓冲
					if (w_len > 0)
					{

					}
					else
					{
						return -1;
					}
					if (r_len < DATA_BUFSIZE)
					{
						//数据读空
						break;
					}
				}
			} while (1);
		}
	}
	return 0;
}

//DWORD WINAPI ProcessIO(LPVOID lpParam)
VOID WINAPI ThreadPoolCallBack(PTP_CALLBACK_INSTANCE instance, PVOID lpParam)
{
	SOCKET app_client = (SOCKET)lpParam;
	SOCKET app_service = 0;

	BYTE err_code=0;
	UCHAR w_buf[10] = {0};

	SafeModule sm;

	err_code = s4init(sm,app_client, app_service);
	if (err_code > 0)
	{
		w_buf[0] = 0;
		w_buf[1] = err_code;
		int len = sendSocketData(app_client, w_buf, 8);
		if (len <= 0)
		{
			return;
		}
		if (err_code != 0x5a)
		{
			return;
		}
		s4DataProcess(sm,app_client, app_service);
	}

	shutdown(app_client, SD_SEND);
	shutdown(app_service, SD_SEND);
	closesocket(app_client);
	closesocket(app_service);
	return;
}

void s4Service()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		OutErr("WSAStartup()");
		WSACleanup();
		return;
	}
	SOCKET sServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
	{
		OutErr("bind");
		WSACleanup();
		return;
	}

	if (listen(sServer, 200) != 0)
	{
		OutErr("listen");
		WSACleanup();
		return;
	}
	cout << "listen on " << PORT  << endl;
	PTP_POOL pool;
	pool = CreateThreadpool(NULL);
	SetThreadpoolThreadMaximum(pool, 150);
	SetThreadpoolThreadMinimum(pool, 20);

	TP_CALLBACK_ENVIRON tcEnv;
	InitializeThreadpoolEnvironment(&tcEnv);
	SetThreadpoolCallbackPool(&tcEnv, pool);

	struct sockaddr_in clientAddr;
	memset(&clientAddr, 0, sizeof(struct sockaddr_in));
	int addr_len = sizeof(struct sockaddr_in);

	while (1)
	{
		SOCKET client = accept(sServer, (SOCKADDR*)&clientAddr, &addr_len);
		if (client == INVALID_SOCKET)
		{
			OutErr("accept");
			WSACleanup();
			return;
		}

		TrySubmitThreadpoolCallback(ThreadPoolCallBack, (PVOID)client, &tcEnv);
#if 0
		HANDLE hProcessIO = CreateThread(NULL, 0, ProcessIO, (LPVOID)client, 0, NULL);
		if (hProcessIO)
		{
			CloseHandle(hProcessIO);
		}
#endif
	}

	closesocket(sServer);
	WSACleanup();
}


int main(int argc,char *argv[])
{

	ParseArgc(argc, argv);

	s4Service();
	return 0;
}


