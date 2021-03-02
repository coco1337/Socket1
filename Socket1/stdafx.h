#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "dbghelp.lib")
#include <iostream>
#include <WinSock2.h>
#include <map>
#include <WS2tcpip.h>
#include <cstring>
#include <process.h>
#include <tchar.h>
#include <timeapi.h>
#include <Windows.h>
#include <MSWSock.h>

#define BUF_SIZE 65536
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 9001
#define GQCS_TIMEOUT INFINITE // 20
#define MAX_CONNECTION 1

/*
 * wireshark
 * (ip.dst == 127.0.0.1) && ((tcp.srcport == 48000) || (tcp.dstport == 48000))
 */