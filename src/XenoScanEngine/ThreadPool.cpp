#include "ThreadPool.h"
#include "ThreadPoolWorker.h"

#include "Assert.h"

#include <chrono>


using namespace std::chrono_literals;

ThreadPool::ThreadPool(float portion)
{
	auto maxThreads = ThreadPool::getMaxThreadCount();
	auto fMaxThreads = (float)maxThreads;
	auto threads = std::min(maxThreads, std::max(1, (int)(fMaxThreads * portion)));
	this->internalInit(threads);
}

ThreadPool::ThreadPool()
{
	this->internalInit(ThreadPool::getMaxThreadCount());
}

void ThreadPool::internalInit(int threadCount)
{
	ASSERT(threadCount <= ThreadPool::getMaxThreadCount());
	ASSERT(threadCount >= 1);

	this->shutdown = false;
	for (int i = 0; i < threadCount; i++)
		this->workers.push_back(std::shared_ptr<ThreadPoolWorker>(new ThreadPoolWorker(this)));
}

ThreadPool::~ThreadPool()
{
	this->shutdown = true;
	this->join();
	this->posted.notify_all();
	this->workers.clear();
}

void ThreadPool::join(std::optional<JoinCallback> callback)
{
	std::unique_lock<std::mutex> lock(this->mutex);
	while (!this->tasks.empty())
	{
		this->posted.notify_all();
		this->completed.wait(lock);
		if (callback.has_value())
		{
			auto size = this->tasks.size();
			lock.unlock();
			callback.value()(size);
			lock.lock();
		}
	}
}

void ThreadPool::execute(Action action)
{
	// don't post the task if we're shutting down
	if (this->shutdown)
		return;

	// otherwise, post and notify
	std::unique_lock<std::mutex> lock(this->mutex);
	this->tasks.push(action);
	this->posted.notify_one();
}

void ThreadPool::notifyWorkComplete()
{
	this->completed.notify_one();
}

std::optional<ThreadPool::Action> ThreadPool::getWork(bool &_shutdown)
{
	// wait until a task is posted
	std::unique_lock<std::mutex> lock(this->mutex);
	if (!this->shutdown && this->tasks.empty())
		this->posted.wait(lock);

	if (this->tasks.empty())
	{
		// task queue is empty, notify for shutdown if needed
		_shutdown = this->shutdown;
		return std::nullopt;
	}

	// pass a task; don't notify for shutdown until all tasks complete
	_shutdown = false;
	auto ret = this->tasks.front();
	this->tasks.pop();
	return ret;
}