#pragma once

#include "Assert.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stack>

template<typename JOB_DETAIL_TYPE>
class StackJobPool
{
public:
	StackJobPool() {}

	void spinup(const size_t totalJobs, const std::string& consoleMessage, const std::function<void(JOB_DETAIL_TYPE& details)>& workerCallback)
	{
		// TODO: add assertions once they're clear

		this->firstUpdate = true;
		this->totalJobs = totalJobs;
		this->updatePrecision = (totalJobs / 1000) + 1;
		this->consoleMessage = consoleMessage;
		this->startTime = std::chrono::high_resolution_clock::now();
		this->numberOfJobsComplete = 0;
		this->iterationIsComplete = false;
		this->workerCallback = workerCallback;

		// find out how many threads we can run
		this->numberOfConcurentThreadsSupported = std::thread::hardware_concurrency();
		if (this->numberOfConcurentThreadsSupported <= 0)
			this->numberOfConcurentThreadsSupported = 2; // TODO: maybe fix

		// set up our job wrapper
		auto workerWrapper = [this]() -> void
		{
			while (!this->iterationIsComplete)
			{
				this->jobLock.lock();

				if (this->pendingJobs.empty())
				{
					this->jobLock.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}

				auto details = this->pendingJobs.top();
				this->pendingJobs.pop();

				this->jobLock.unlock();

				this->workerCallback(details);

				this->numberOfJobsComplete++;
			}
		};

		// fire up the threads; 1 fewer since our caller will also be running
		for (size_t i = 0; i < this->numberOfConcurentThreadsSupported - 1; i++)
			this->workerThreads.push_back(std::thread(workerWrapper));
	}

	inline void addJob(const JOB_DETAIL_TYPE job)
	{
		// TODO: add assertions once they're clear

		this->jobLock.lock();
		this->pendingJobs.push(job);
		this->jobLock.unlock();

		if (this->numberOfJobsComplete % this->updatePrecision == 0)
			this->updateConsole();
	}

	inline void incrementCompletionCount()
	{
		this->numberOfJobsComplete++;
	}

	void waitForCompletion()
	{
		this->jobLock.lock();
		while (!this->pendingJobs.empty())
		{
			this->jobLock.unlock();

			this->updateConsole();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			this->jobLock.lock();
		}
		this->jobLock.unlock();

		this->iterationIsComplete = true;

		for (auto thread = this->workerThreads.begin(); thread != this->workerThreads.end(); thread++)
			thread->join();
		this->workerThreads.clear();
		this->updateConsole(true);
	}


private:
	std::function<void(JOB_DETAIL_TYPE& details)> workerCallback;
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	size_t numberOfConcurentThreadsSupported;
	std::vector<std::thread> workerThreads;
	std::stack<JOB_DETAIL_TYPE> pendingJobs;
	std::atomic<size_t> numberOfJobsComplete;
	std::atomic<bool> iterationIsComplete;
	std::mutex jobLock;

	std::string consoleMessage;
	size_t totalJobs;
	size_t updatePrecision;
	bool firstUpdate;
	size_t backtrackChars;

	inline void updateConsole(bool final = false)
	{
		if (this->totalJobs == 0 || this->consoleMessage.length() == 0)
			return;

		if (this->firstUpdate)
		{
			this->firstUpdate = false;
			this->backtrackChars = 0;
			std::cout
				<< this->numberOfConcurentThreadsSupported
				<< " Threads Scanning - "
				<< this->consoleMessage << " ";
		}

		std::stringstream eraser;
		for (size_t i = 0; i < this->backtrackChars; i++)
			eraser << '\b';
		std::cout << eraser.str();

		std::stringstream msg;
		msg << this->numberOfJobsComplete << " of " << this->totalJobs;
		std::cout << msg.str();
		this->backtrackChars = msg.str().length();

		if (final)
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto diff = end - this->startTime;
			std::cout << " (Time " << std::chrono::duration<double, std::milli>(diff).count() << "ms)" << std::endl;
		}

	}
};