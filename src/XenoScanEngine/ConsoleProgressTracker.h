#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>

class ConsoleProgressTracker
{
public:
	ConsoleProgressTracker(const std::string& _taskDescription, size_t _numberOfThreads, size_t _numberOfTasks, size_t _updatePrecision)
		: taskDescription(_taskDescription), numberOfThreads(_numberOfThreads), numberOfTasks(_numberOfTasks),
		updatePrecision(_updatePrecision), numberOfCompleteTasks(0), lastUpdateNumberCompleteTasks(0),
		numberOfCharsToBacktrack(0), done(false)
	{
		this->startTime = std::chrono::high_resolution_clock::now();
		std::cout << this->numberOfThreads << " Threads Scanning - " << this->taskDescription << " ";
		this->updateConsole();
	}

	~ConsoleProgressTracker()
	{
		// treat everything as done once we're destroyed so we
		// print out the final result
		this->setNumberOfCompleteTasks(this->numberOfTasks);
	}

	void setNumberOfCompleteTasks(size_t amount)
	{
		this->numberOfCompleteTasks = amount;
		auto delta = this->numberOfCompleteTasks - this->lastUpdateNumberCompleteTasks;
		if (delta > this->updatePrecision || this->numberOfCompleteTasks == this->numberOfTasks)
			this->updateConsole();
	}

private:
	std::string taskDescription;
	size_t numberOfThreads, numberOfTasks, numberOfCompleteTasks;
	size_t updatePrecision, lastUpdateNumberCompleteTasks;
	size_t numberOfCharsToBacktrack;

	bool done;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	void updateConsole()
	{
		if (this->done) return;

		// prepend our output with however many chars we need to erase old info
		std::stringstream message;
		for (size_t i = 0; i < this->numberOfCharsToBacktrack; i++)
			message << '\b';
		
		// then add the new info
		message << this->numberOfCompleteTasks << " of " << this->numberOfTasks;

		// print the info and track any relevant data
		auto output = message.str();
		std::cout << output;
		this->numberOfCharsToBacktrack = output.length() - this->numberOfCharsToBacktrack;
		this->lastUpdateNumberCompleteTasks = this->numberOfCompleteTasks;

		// if we're totally done
		if (this->numberOfCompleteTasks == this->numberOfTasks)
		{
			this->done = true;
			auto end = std::chrono::high_resolution_clock::now();
			auto diff = end - this->startTime;
			std::cout << " (Time " << std::chrono::duration<double, std::milli>(diff).count() << "ms)" << std::endl;
		}
	}
};