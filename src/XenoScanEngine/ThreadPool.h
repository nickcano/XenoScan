#pragma once

#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <optional>
#include <condition_variable>

class ThreadPool
{
public:
	typedef std::function<void()> Action;
	typedef std::function<void(size_t)> JoinCallback;

	ThreadPool(float portion);
	ThreadPool();
	~ThreadPool();

	void join(std::optional<JoinCallback> callback = std::nullopt);
	void execute(Action action);
	size_t getNumberOfWorkers()
	{
		return this->workers.size();
	}

	static int getMaxThreadCount()
	{
		return std::thread::hardware_concurrency();
	}

protected:
	friend class ThreadPoolWorker;

	void notifyWorkComplete();
	std::optional<Action> getWork(bool &shutdown);

private:
	std::mutex mutex;
	std::queue<Action> tasks;
	std::atomic<bool> shutdown;
	std::condition_variable posted, completed;
	std::vector<std::shared_ptr<ThreadPoolWorker>> workers;

	void internalInit(int threadCount);
};