  
#include <stdio.h>   
#include <conio.h> 
#include <winsock2.h>
#include <process.h>   
#include <stdlib.h>
#include <windows.h>   
#include <time.h>

#include "package.h"

#pragma comment(lib, "ws2_32.lib")

#define BUFSIZE 256
#define PORT 5730


SOCKET sServer;
SOCKET sAccept[1000];

int flag = 0;
int offset = 0;
int quit = 0;
HANDLE handle[1000];
struct sockaddr_in clientAddr[1000]; //客户端地址

									 //子线程函数   
unsigned int __stdcall ThreadFun(PVOID pM)
{
	int bufVal, ret;
	int stream_left;
	pkg_header inst;
	char *inst_ptr;
	int id = (int)pM;
	char *buf, *buf_ptr, *buf2;
	SYSTEMTIME sys;
	char buffer[256];
	//printf("Client %d, %s:%d connected\n", id, inet_ntoa(clientAddr[id].sin_addr), (int)clientAddr[id].sin_port);
	while (1) {
		if (quit)
		{
			closesocket(sAccept[id]);
			sAccept[id] = NULL;
			printf("Close connection with client %d\n", id);
			return 0;
		}
		stream_left = sizeof(pkg_header);
		inst_ptr = (char *)&inst;
		while (stream_left > 0)
		{
			ret = recv(sAccept[id], inst_ptr, stream_left, 0);
			if (ret == SOCKET_ERROR)
			{
				printf("Client %d, WSA Error: %d\n", id, WSAGetLastError());
				closesocket(sAccept[id]);
				sAccept[id] = NULL;
				return 0;
			}
			if (ret == 0)
			{
				printf("Connection with Client %d closed!\n", id);
				closesocket(sAccept[id]);
				sAccept[id] = NULL;
				return 0;
			}
			stream_left -= ret;
			inst_ptr += ret;
		}
		if (stream_left == 0)
		{
			printf("type:%d len:%d ", inst.type, inst.length);
		}
		if (inst.type == TYPE_TIME) //time命令，查看服务器时间
		{
			char myTime[256];
			GetLocalTime(&sys);
			sprintf(myTime, "%4d/%02d/%02d %02d:%02d:%02d.%03d", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
			printf("Client %d %s:time\n",id, inet_ntoa(clientAddr[id].sin_addr));
			inst.type = TYPE_TIME;
			inst.length = strlen(myTime) + 1;
			inst.target = id;
			send(sAccept[id], (char*)&inst, sizeof(inst), 0);
			send(sAccept[id], myTime, inst.length, 0);
		}
		else if (inst.type == TYPE_NAME)//name命令，查看主机名称
		{
			char hostname[256];
			int res = gethostname(hostname, sizeof(hostname));
			printf("Client %d %s:name\n", id, inet_ntoa(clientAddr[id].sin_addr));
			inst.type = TYPE_NAME;
			inst.length = strlen(hostname) + 1;
			inst.target = id;
			send(sAccept[id], (char*)&inst, sizeof(inst), 0);
			send(sAccept[id], hostname, inst.length, 0);
		}
		else if (inst.type == TYPE_CLIENTLIST) //list命令
		{
			int port = 0;
			char info[1001] = "";
			char tmp[100];
			inst.target = id;
			printf("Client %d %s:list\n", id,inet_ntoa(clientAddr[id].sin_addr));
			for (int i = 0; i < offset; i++)
			{
				if (sAccept[i] == NULL)
				{
					continue;
				}
				memset(tmp, 0, 100);
				port = (int)clientAddr[i].sin_port;
				if (i == id)
				{
					sprintf(tmp, "Client *%d %s:%d\n", i, inet_ntoa(clientAddr[i].sin_addr), port);
				}
				else
				{
					sprintf(tmp, "Client  %d %s:%d\n", i, inet_ntoa(clientAddr[i].sin_addr), port);
				}
				strcat(info, tmp);
			}
			inst.type = TYPE_CLIENTLIST;
			inst.length = strlen(info) + 1;
			send(sAccept[id], (char*)&inst, sizeof(inst), 0);
			send(sAccept[id], info, inst.length, 0);
		}
		else if (inst.type == TYPE_MESSAGE) //send命令
		{
			buf = (char *)malloc(256 + inst.length * sizeof(char));
			buf_ptr = buf;
			stream_left = inst.length;
			printf("%s:msg\n", inet_ntoa(clientAddr[id].sin_addr));
			while (stream_left > 0)
			{
				ret = recv(sAccept[id], buf_ptr, stream_left, 0);
				if (ret == SOCKET_ERROR)
				{
					printf("Client %d, WSA Error: %d\n", id, WSAGetLastError());
					closesocket(sAccept[id]);
					sAccept[id] = NULL;
					return 0;
				}
				if (ret == 0)
				{
					printf("Connection with Client %d closed!\n",id);
					closesocket(sAccept[id]);
					sAccept[id] = NULL;
					return 0;
				}
				stream_left -= ret;
				buf_ptr += ret;
			}
			inst.source = id;
			printf("msg: %s F:%d T:%d", buf, inst.source, inst.target);
			if (sAccept[inst.target] != NULL)
			{
				sprintf(buffer, "\nMessage From Client %d, %s:%d", id, inet_ntoa(clientAddr[id].sin_addr), (int)clientAddr[id].sin_port);
				strcat(buf, buffer);
				inst.length = strlen(buf) + 1;
				send(sAccept[inst.target], (char*)&inst, sizeof(inst), 0);
				send(sAccept[inst.target], buf, inst.length, 0);
			}
			else
			{
				buf2 = (char *)malloc(256 + inst.length * sizeof(char));
				inst.type = TYPE_FAIL;
				sprintf(buf2, "Cannot send message '%s' to client %d, client %d not exist!", buf, inst.target, inst.target);
				inst.length = strlen(buf2) + 1;
				send(sAccept[id], (char*)&inst, sizeof(inst), 0);
				send(sAccept[id], buf2, inst.length, 0);
				free(buf2);
			}
			free(buf);
		}
		else if (inst.type == TYPE_FEEDBACK)
		{
			printf("%s:feedback F:%d T:%d\n", inet_ntoa(clientAddr[id].sin_addr), inst.source, inst.target);
			inst.length = 0;
			send(sAccept[inst.target], (char*)&inst, sizeof(inst), 0);
		}
	}
	closesocket(sAccept[id]);
	sAccept[id] = NULL;
	return 0;
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	int i;
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
		printf("Closing server...\n");
		quit = 1;
		for (i = 0; i < offset; i++)
		{
			if (sAccept[i] != NULL)
			{
				shutdown(sAccept[i], SD_BOTH);
			}
		}
		Sleep(5000);
		for (i = 0; i < offset; i++)
		{
			if (sAccept[i] != NULL)
			{
				closesocket(sAccept[i]);
				sAccept[i] = NULL;
			}
		}
		Sleep(1000);
		for (i = 0; i < offset; i++)
		{
			if (sAccept[i] != NULL)
			{
				printf("Cannot close connection with client %d, please try again", i);
				return TRUE;
			}
		}
		closesocket(sServer);
		WSACleanup();
		exit(0);
	default:
		return FALSE;
	}
}

//主函数，所谓主函数其实就是主线程执行的函数。   
int main()
{
	WSADATA wsaData;
	//WSAStartup()初始化Winsock 2.2, 返回WSADATA结构体
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("initialize error\n");
		return 0;
	}
	//创建TCP Socket，使用AF_INET协议族（ ipv4地址（ 32位的）与端口号（ 16位的）），使用流式套接字
	sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sServer == INVALID_SOCKET)
		printf("create socket error\n");
	//绑定地址
	struct sockaddr_in addr;
	int addr_len = sizeof(struct sockaddr_in);
	int port = PORT;
	int errCode;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port); //端口是16位
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //提供任意适合的网络地址
											  //绑定Socket
	errCode = bind(sServer, (SOCKADDR *)&addr, addr_len);
	if (errCode == SOCKET_ERROR)
	{
		printf("bind error\n");
		closesocket(sServer);
		WSACleanup();
		exit(1);
	}
	//const int THREAD_NUM = 1000;
	
	errCode = listen(sServer, SOMAXCONN);
	if (errCode == SOCKET_ERROR)
	{
		printf("LISTEN ERROR\n");
		closesocket(sServer);
		WSACleanup();
		exit(1);
	}
	printf("Server started. Listen on 5730. No client now.\n");
	int temp, i;

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	while (1) //持续监听客户端
	{
		if (quit)
		{
			break;
		}
		//与客户端建立连接
		int clientAddrlen = sizeof(clientAddr[0]);
		if (offset == 1000)
		{
			for (i = 0; i < 1000; i++)
			{
				if (sAccept[i] == NULL)
				{
					temp = i;
					break;
				}
			}
			if (i == 1000)
			{
				printf("Can not handle more connection!\n");
				continue;
			}
		}
		else
		{
			temp = offset;
		}
		sAccept[temp] = accept(sServer, (SOCKADDR*)&clientAddr[temp], &clientAddrlen);
		if (sAccept[temp] == INVALID_SOCKET)
		{
			printf("ACCEPT ERROR:%d", WSAGetLastError());
			closesocket(sAccept[temp]);
			sAccept[temp] = NULL;
		}
		else
		{
			handle[temp] = (HANDLE)_beginthreadex(NULL, 0, ThreadFun, (void *)temp, 0, NULL);
			printf("Client %d %s:%d connected\n",temp, inet_ntoa(clientAddr[temp].sin_addr), clientAddr[temp].sin_port); //连接的客户端信息
			//send(sAccept[temp], "Server connected!\n", strlen("Server connected!\n") + 1, 0); //发送连接成功信息
			if (offset < 1000)
				offset++;
		}
	}
	if (offset > 0)
	{
		WaitForMultipleObjects(offset, handle, TRUE, INFINITE);
	}
	WSACleanup();
	return 0;
}
