#pragma once

#include <thread>

class ThreadPoolWorker
{
public:
	~ThreadPoolWorker();

	ThreadPoolWorker() = delete;
	ThreadPoolWorker(const ThreadPoolWorker&) = delete;
	ThreadPoolWorker& operator=(const ThreadPoolWorker&) = delete;

protected:
	friend class ThreadPool;

	ThreadPoolWorker(ThreadPool* executor);

private:
	std::thread thread;
	ThreadPool* parentExecutor;
};