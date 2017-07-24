#include "Scanner.h"
#include "ScannerTarget.h"
#include "DataStructureBlueprint.h"
#include "Assert.h"

#include <algorithm>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stack>

Scanner::Scanner() : scanState(nullptr)
{
	this->typeRangeMap[SCAN_INFER_TYPE_ALL_TYPES] = typeRange(ScanVariant::SCAN_VARIANT_ALLTYPES_BEGIN, ScanVariant::SCAN_VARIANT_ALLTYPES_END);
	this->typeRangeMap[SCAN_INFER_TYPE_STRING_TYPES] = typeRange(ScanVariant::SCAN_VARIANT_STRINGTYPES_BEGIN, ScanVariant::SCAN_VARIANT_STRINGTYPES_END);
	this->typeRangeMap[SCAN_INFER_TYPE_NUMERIC_TYPES] = typeRange(ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN, ScanVariant::SCAN_VARIANT_NUMERICTYPES_END);

	this->scanState.reset(new ScanState());
}
Scanner::~Scanner()
{
}

void Scanner::startNewScan()
{
	this->scanState->clearScanResults();
}

void Scanner::runScan(const ScannerTargetShPtr &target, const ScanVariant &needle, const CompareTypeFlags &comp, const ScanInferType &type)
{
	ASSERT(target.get() != nullptr);
	ASSERT(this->scanState.get() != nullptr);
	ASSERT(comp >= SCAN_COMPARE_BEGIN && comp <= SCAN_COMPARE_END);
	ASSERT(comp >= SCAN_INFER_TYPE_ALL_TYPES && comp <= SCAN_INFER_TYPE_END);

	ScanResultCollection needles;
	if (type == SCAN_INFER_TYPE_EXACT)
		needles.push_back(needle);
	else
	{
		auto rawValue = needle.toString();
		auto range = this->typeRangeMap[type];
		for (size_t i = range.low; i <= range.high; i++)
		{
			auto val = ScanVariant::FromStringTyped(rawValue, i);
			if (!val.isNull())
				needles.push_back(val);
		}
	}

	if (this->scanState->isFirstScan())
		this->doScan(target, needles, comp);
	else
		this->doReScan(target, needles, comp);
}

void Scanner::runDataStructureScan(const ScannerTargetShPtr &target, const std::string &type)
{
	ASSERT(target.get() != nullptr);
	this->doDataStructureScan(target, type);
}

MemoryInformationCollection Scanner::getScannableBlocks(const ScannerTargetShPtr &target) const
{
	auto startAddress = target->getLowestAddress();
	auto endAdress = target->getHighestAddress();

	auto nextAddress = startAddress;

	MemoryInformationCollection blocks;
	while (nextAddress < endAdress)
	{
		MemoryInformation meminfo;
		if (target->queryMemory(nextAddress, meminfo, nextAddress))
			if (meminfo.isCommitted && !meminfo.isMirror)
				blocks.push_back(meminfo);
	}

	return blocks;
}

void Scanner::calculateBoundsOfBlocks(const ScannerTargetShPtr &target, const MemoryInformationCollection &blocks, MemoryAddress &lower, MemoryAddress &upper) const
{
	// these are swapped because we're min/maxing based on blocks
	lower = target->getHighestAddress();
	upper = target->getLowestAddress();
	for (auto block = blocks.cbegin(); block != blocks.cend(); block++)
	{
		auto endAddress = (MemoryAddress)((size_t)block->allocationBase + block->allocationSize);
		if (block->allocationBase < lower)
			lower = block->allocationBase;
		else if (endAddress > upper)
			upper = endAddress;
	}
}

void Scanner::iterateOverBlocks(const ScannerTargetShPtr &target, const MemoryInformationCollection &blocks, blockIterationCallback callback) const
{
	// find out how many threads we can run
	auto concurentThreadsSupported = std::thread::hardware_concurrency();
	if (concurentThreadsSupported <= 0)
		concurentThreadsSupported = 2; // TODO: maybe fix

	// define some common variables
	auto start = std::chrono::high_resolution_clock::now();
	struct QueuedBlock
	{
		size_t size;
		uint8_t* buffer;
		MemoryAddress baseAddress;
	};
	std::vector<std::thread> workerThreads;
	std::stack<QueuedBlock> blocksToProcess;
	std::atomic<bool> iterationComplete(false);
	std::atomic<size_t> completedBlocks = 0;
	std::mutex blockLock;

	// this is just used for console output,
	// makes debugging easier
	std::string blocksMessage = "";
	std::cout << concurentThreadsSupported << " Threads Scanning - Block ";

	auto updateConsole = [&]() -> void
	{
		std::stringstream eraser;
		for (size_t i = 0; i < blocksMessage.length(); i++)
			eraser << '\b';
		std::cout << eraser.str();

		std::stringstream msg;
		msg << completedBlocks << " of " << blocks.size();
		blocksMessage = msg.str();
		std::cout << blocksMessage;
	};

	// define our iteration function
	auto threadedIterate = [&]() -> void
	{
		while (!iterationComplete)
		{
			// let's get a block to process
			blockLock.lock();

			if (blocksToProcess.empty())
			{
				blockLock.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			auto block = blocksToProcess.top();
			blocksToProcess.pop();

			blockLock.unlock();

			// now lets process it
			callback(block.baseAddress, block.buffer, block.size);
			delete[] block.buffer;
			block.buffer = nullptr;

			// mark it as complete TODO try not to aqcuire lock for this
			completedBlocks++;
		}
	};

	// fire up the threads; 1 fewer since we're still going too
	for (size_t i = 0; i < concurentThreadsSupported - 1; i++)
		workerThreads.push_back(std::thread(threadedIterate));


	// start giving the threads stuff to do
	for (auto block = blocks.cbegin(); block != blocks.cend(); block++)
	{
		// queue up this block for scanning
		auto buffer = new uint8_t[block->allocationSize];
		while (!buffer)
		{
			// we're probably hogging  too much memory, give threads some time to spin
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			buffer = new uint8_t[block->allocationSize];
		}

		QueuedBlock qb;
		qb.buffer = buffer;
		qb.size = block->allocationSize;
		qb.baseAddress = block->allocationBase;

		if (!target->readArray<uint8_t>(qb.baseAddress, qb.size, qb.buffer))
		{
			// failures will typically happen when target isn't frozen, as it's
			// memory allocations will change between the start and end of the scan
			delete[] qb.buffer;
			completedBlocks++;
			continue;
		}

		blockLock.lock();
		blocksToProcess.push(qb);
		blockLock.unlock();

		updateConsole();
	}

	blockLock.lock();
	while (!blocksToProcess.empty())
	{
		blockLock.unlock();

		updateConsole();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		blockLock.lock();
	}
	blockLock.unlock();

	iterationComplete = true;

	for (auto thread = workerThreads.begin(); thread != workerThreads.end(); thread++)
		thread->join();
	workerThreads.clear();

	updateConsole();
	auto end = std::chrono::high_resolution_clock::now();
	auto diff = end - start;
	std::cout << " (Time " << std::chrono::duration<double, std::milli>(diff).count() << "ms)" << std::endl;
}

void Scanner::doScan(const ScannerTargetShPtr &target, const ScanResultCollection &needles, const CompareTypeFlags &compType)
{
	// determine which blocks of memory can be scanned
	auto blocks = this->getScannableBlocks(target);

	// helper lambda that takes care of scanning each chunk
	std::mutex mutex;
	ScanResultMap results;
	ScanResultAddressAllocator resLocAllocator;
	bool isLittleEndian = target->isLittleEndian();
	auto scanChunk = [needles, compType, isLittleEndian, &mutex, &resLocAllocator, &results]
					(const MemoryAddress &baseAddress, const uint8_t* chunk, const size_t &chunkSize)
					-> void
	{
		// for each needle generated, see if it's in the chunk
		std::vector<size_t> locations;
		for (auto needle = needles.cbegin(); needle != needles.cend(); needle++)
		{
			locations.clear();
			needle->searchForMatchesInChunk(chunk, chunkSize, compType, baseAddress, isLittleEndian, locations);
			for (auto loc = locations.cbegin(); loc != locations.cend(); loc++)
			{
				auto resultLoc =
					std::allocate_shared
						<
							ScanResultAddress,
							ScanResultAddressAllocator
						>
						(
							resLocAllocator,
							(MemoryAddress)((size_t)baseAddress + *loc)
						);

				mutex.lock();
				auto found = results.find(resultLoc);
				if (found == results.end())
				{
					ScanResultCollection temp;
					temp.push_back(ScanVariant::FromRawBuffer(&chunk[*loc], chunkSize - *loc, isLittleEndian, *needle));
					results.emplace(std::make_pair(resultLoc, temp));
				}
				else
					found->second.push_back(*needle);
				mutex.unlock();
			}
		}
	};

	this->iterateOverBlocks(target, blocks, scanChunk);
	this->scanState->updateState(results);
}

void Scanner::doReScan(const ScannerTargetShPtr &target, const ScanResultCollection &needles, const CompareTypeFlags &compType)
{
	ScanResultMap newResults;

	//TODO: If re-scans search for a string of a size that is larger than the initial scan, and it somehow surpasses 0x1000 in size, 
	// this will end badly. Fix.
	size_t bufferSize = 0x1000;
	uint8_t* buffer = new uint8_t[bufferSize];
	bool isLittleEndian = target->isLittleEndian();
	for (auto resultLocation = this->scanState->beginResult(); resultLocation != this->scanState->endResult(); resultLocation++)
	{
		size_t bytesToRead = 0;
		ScanResultCollection searchNeedles;
		for (auto result = resultLocation->second.begin(); result != resultLocation->second.end(); result++)
		{
			for (auto needle = needles.begin(); needle != needles.end(); needle++)
			{
				if (result->getType() == needle->getType())
				{
					bytesToRead = std::max(bytesToRead, result->getSize());
					searchNeedles.push_back(*needle);
				}
			}
		}

		if (bytesToRead > bufferSize)
		{
			bufferSize = bytesToRead;
			delete [] buffer;
			buffer = new uint8_t[bufferSize];
		}

		if (!resultLocation->first->readCurrentValue(target, buffer, bytesToRead))
			continue;

		ScanResultCollection newResultValues;

		for (auto needle = searchNeedles.begin(); needle != searchNeedles.end(); needle++)
		{
			auto res = needle->compareTo(buffer, isLittleEndian);
			if ((res & compType) != 0)
				newResultValues.push_back(ScanVariant::FromRawBuffer(buffer, bufferSize, isLittleEndian, *needle));
		}

		if (newResultValues.size())
			newResults.insert(std::make_pair(resultLocation->first, newResultValues));
	}
	delete [] buffer;
	this->scanState->updateState(newResults);
}

void Scanner::doDataStructureScan(const ScannerTargetShPtr &target, const std::string &type)
{
	auto supported = target->getSupportedBlueprints();
	if (supported.find(type) == supported.cend())
	{
		std::cout << "Data Structure Blueprint type not supported: '" << type << "'!" << std::endl;
		return;
	}


	// determine which blocks of memory can be scanned
	auto blocks = this->getScannableBlocks(target);

	// calculate block bounds, this will help speed up the pointer locator
	MemoryAddress upperBound, lowerBound;
	this->calculateBoundsOfBlocks(target, blocks, lowerBound, upperBound);

	// first, we need to scan through every block and find any values which seem
	// to be valid pointers within the target. We use a map to dedupe, since,
	// even if a pointer appears multiple times, we only need to scan it once
	std::mutex mutex;
	PointerMap foundPointers;
	auto findPointers = [this, upperBound, lowerBound, &target, &blocks, &mutex, &foundPointers]
						(const MemoryAddress &baseAddress, const uint8_t* chunk, const size_t &chunkSize)
						-> void
	{
		// TODO: might have to fix this for platforms with different address sizes
		size_t desiredAlignment = target->getPointerSize();
		size_t chunkAlignment = (size_t)baseAddress % desiredAlignment;
		size_t startOffset = (chunkAlignment == 0) ? 0 : desiredAlignment - chunkAlignment;
		size_t thingsToScan = (chunkSize - startOffset) / desiredAlignment;

		auto pointersToCheck = reinterpret_cast<const MemoryAddress*>(&chunk[startOffset]);
		for (size_t i = 0; i < thingsToScan; i++)
		{
			auto check = pointersToCheck[i];
			if (this->isValidPointer(lowerBound, upperBound, check) && ((size_t)check % desiredAlignment) == 0)
			{
				auto location = (MemoryAddress)
				(
					(size_t)startOffset + (size_t)baseAddress + i * desiredAlignment
				);

				mutex.lock();
				foundPointers[check].push_back(location);
				mutex.unlock();
			}
		}
	};
	this->iterateOverBlocks(target, blocks, findPointers);

	// with the list of pointers, scan for valid structures
	DataStructureResultMap results;
	DataStructureBlueprint::findDataStructures(target, type, foundPointers, results);
	this->scanState->updateState(results);
}