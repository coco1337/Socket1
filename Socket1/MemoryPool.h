#pragma once

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo :SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize(size), mExtraInfo(-1)
	{}

	long mAllocSize;	// MemAllocInfo�� ���Ե� ũ��
	long mExtraInfo;	// ��Ÿ �߰� ����
};	// total 16 byte

inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
{
	// TODO : header �� MemAllocInfo�� ��ģ ���� ���� �ۿ��� ����� �޸� �ּҸ� void*�� ����
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
		// ��� �� �ٲٱ�
		MAX_SMALL_POOL_COUNT = 1024 / 32 + 1024 / 128 + 2048 / 256,
		MAX_ALLOC_SIZE = 4096
	};

	// ���ϴ� ũ���� �޸𸮸� ������ �ִ� Ǯ�� O(1) access�� ���� ���̺�
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[MAX_ALLOC_SIZE + 1];
};

extern MemoryPool* GMemoryPool;

struct PooledAllocatable {};

template <class T, class... Args>
T* xnew(Args... arg)
{
	static_assert(true == std::isConvertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");
	// �޸� �Ҵ�
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);	
}

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");
	// �Ҹ��� �ҷ��ְ� �޸� �ݳ�
	object->~T();
	GMemoryPool->Deallocate(object, sizeof(T));
}