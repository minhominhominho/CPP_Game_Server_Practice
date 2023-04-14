#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

/*---------------------
	  JobQueue
----------------------*/

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK

	// ù��° Job�� ���� �����尡 ������� ���
	if (prevCount == 0)
	{
		// �̹� �������� JobQueue�� ������ ����
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// ���� �ִ� �ٸ� �����尡 �����ϵ��� GlobalQueue�� �ѱ��
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

// 1) �ϰ��� �ʹ� ������? -> �� �����忡 �����ϰ� ���ư��� �� �� ����
// 2) DoAsync�� Ÿ�� Ÿ�� ���� ���� ������ �ȴ� ��Ȳ (�ϰ��� �� ���������� ����)
void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		Vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs); // WRITE_LOCK

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Excute();

		// ���� �ϰ��� 0����� ����
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;

			// ���� �ִ� �ٸ� �����尡 �����ϵ��� GlobalQueue�� �ѱ��
			GGlobalQueue->Push(shared_from_this());
			break;
		}
	}

}