#include "ThreadPoolWorker.h"
#include "ThreadPool.h"


ThreadPoolWorker::ThreadPoolWorker(ThreadPool* executor)
	: parentExecutor(executor)
{
	this->thread = std::thread([this]() -> void
	{
		bool shutdown = false;
		while (!shutdown)
		{
			auto task = this->parentExecutor->getWork(shutdown);
			if (task.has_value())
				task.value()();
			this->parentExecutor->notifyWorkComplete();
		}
	});
}

ThreadPoolWorker::~ThreadPoolWorker()
{
	this->thread.join();
	this->parentExecutor = nullptr;
}