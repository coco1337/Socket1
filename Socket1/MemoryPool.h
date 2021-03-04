#pragma once

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo :SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize(size), mExtraInfo(-1)
	{}

	long mAllocSize;	// MemAllocInfo가 포함된 크기
	long mExtraInfo;	// 기타 추가 정보
};	// total 16 byte

inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
{
	// TODO : header 에 MemAllocInfo를 펼친 다음 실제 앱에서 사용할 메모리 주소를 void*로 리턴
	new (header)MemAllocInfo(size);
	return reinterpret_cast<void*>(++header);
}

inline MemAllocInfo* DetachMemAllocInfo(void* ptr)
{
	MemAllocInfo* header = reinterpret_cast<MemAllocInfo*>(ptr);
	--header;
	return header;
}

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
class SmallSizeMemoryPool
{
public:
	SmallSizeMemoryPool(DWORD allocSize);
	
	MemAllocInfo* Pop();
	void Push(MemAllocInfo* ptr);

private:
	SLIST_HEADER mFreeList;
	
	const DWORD mAllocSize;
	volatile long mAllocCount = 0;
};

class MemoryPool
{
public:
	MemoryPool();
	void* Allocate(int size);
	void Deallocate(void* ptr, long extraInfo);

private:
	enum Config
	{
		// 계산 후 바꾸기
		MAX_SMALL_POOL_COUNT = 1024 / 32 + 1024 / 128 + 2048 / 256,
		MAX_ALLOC_SIZE = 4096
	};

	// 원하는 크기의 메모리를 가지고 있는 풀에 O(1) access를 위한 테이블
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[MAX_ALLOC_SIZE + 1];
};

extern MemoryPool* GMemoryPool;

struct PooledAllocatable {};

template <class T, class... Args>
T* xnew(Args... arg)
{
	static_assert(true == std::isConvertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");
	// 메모리 할당
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);	
}

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");
	// 소멸자 불러주고 메모리 반납
	object->~T();
	GMemoryPool->Deallocate(object, sizeof(T));
}