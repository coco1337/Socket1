#pragma once
#define LISTEN_PORT 48000

enum THREAD_TYPE
{
	THREAD_MAIN_ACCEPT,
	THREAD_IO_WORKER
};

enum RW_MODE {
	READ,
	WRITE
};

extern __declspec(thread) int LThreadType;