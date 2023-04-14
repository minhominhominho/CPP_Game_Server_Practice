#include "pch.h"
#include "MemoryPool.h"

/*---------------------
	MemoryPool
----------------------*/

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	::InitializeSListHead(&_header);
}

MemoryPool::~MemoryPool()
{
	// '=' �� �ϳ��� ���� -> ������ �����ؼ� ���׿� �ְ� ������ üũ�ϴ� ����
	while (MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header)))
		::_aligned_free(memory);
}

void MemoryPool::Push(MemoryHeader* ptr)
{
	ptr->allocSize = 0;

	// Pool�� �޸� �ݳ�
	::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

	_useCount.fetch_sub(1);
	_reserveCount.fetch_add(1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));

	// ������ ���� ����� -> �������� �Ҵ�
	// �ƿ� ���� �ܰ迡�� �Ҵ��ϴ� ����� ����
	if (memory == nullptr)
	{
		// 16byte�� ���߱� ���� _aligned_malloc ����
		memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
		// Stomp Allocator�� ������� �ʴ� ����:
		// Stomp Allocator�� UAF�� �����ϱ� �����̾��µ�,
		// �޸� Ǯ���� ������ �����̱� ������
		// ���� ����� ������ ����
	}
	else // Ȥ�ó� �ϴ� ���� üũ
	{
		// Push���� queue�� �ݳ��Ҷ��� allocSize 0���� ���� �ݳ�
		ASSERT_CRASH(memory->allocSize == 0);
		_reserveCount.fetch_sub(1);
	}

	_useCount.fetch_add(1);

	return memory;
}