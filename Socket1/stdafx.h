#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment (lib, "winmm.lib")
#include <iostream>
#include <WinSock2.h>
#include <map>
#include <WS2tcpip.h>
#include <cstring>
#include <process.h>
#include <tchar.h>
#include <timeapi.h>
#include <Windows.h>

#define BUF_SIZE 4096
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 48000
#define GQCS_TIMEOUT 20

/*
 * wireshark
 * (ip.dst == 127.0.0.1) && (((tcp.dstport >= 27000 && tcp.dstport <= 30000) && (tcp.srcport == 48000)) || ((tcp.dstport == 48000) && (tcp.srcport >= 27000 && tcp.srcport <= 30000)))
 */