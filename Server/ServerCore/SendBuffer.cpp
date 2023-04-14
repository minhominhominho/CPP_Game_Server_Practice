#include "pch.h"
#include "SendBuffer.h"

/*---------------------
	   SendBuffer
----------------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*---------------------
	SendBufferChunk
----------------------*/

// SendBufferChunk�� �Լ����� ��� TLS ���� ���ο��� ����� ����
SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
	ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize())
		return nullptr;

	_open = true;
	return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_open == true);
	_open = false;
	_usedSize += writeSize;
}

/*---------------------
	SendBufferManager
----------------------*/

SendBufferRef SendBufferManager::Open(uint32 size)
{
	// ���������� �ܺο��� ����, �Ҵ��� ��û�ϴ� �Լ�

	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop();	// WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	// �� ������ ������ ���ŷ� ��ü
	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop();	// WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	// cout << "Free : " << LSendBufferChunk->FreeSize() << endl;

	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	//cout << "Pop SENDBUFFERCHUNK" << endl;

	{
		// �������� ������, ������ ��
		WRITE_LOCK;
		if (_sendBufferChunks.empty() == false)
		{
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	// �������� ���ٸ�, ���� ����� ��
		// MakeShared�� �Ҹ��ϸ� �ش� �Ҹ��ڸ� ȣ�����ִ� ������� ��������
		// shared_ptr�� �̿��� deleter�� �����ؼ� �Ҹ��� ���� ������ PushGlobal�� �Ѱ���
	return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
	WRITE_LOCK;
	_sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	cout << "PushGlobal SENDBUFFERCHUNK" << endl;

	GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}