#pragma once
#pragma comment(lib,"ws2_32.lib")
#include <iostream>
#include <WinSock2.h>
#include <map>
#include <WS2tcpip.h>
#include <cstring>
#include <process.h>
#include <tchar.h>

#define BUF_SIZE 4096
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 48000
#define GQCS_TIMEOUT 20;