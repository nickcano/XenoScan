#include "Scanner.h"
#include "ScannerTarget.h"
#include "DataStructureBlueprint.h"
#include "Assert.h"

#include <algorithm>
#include <sstream>

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
			auto val = ScanVariant::fromString(rawValue, i);
			if (!val.isNull())
				needles.push_back(val);
		}
	}

	if (this->scanState->isFirstScan())
		this->doScan(target, needles, comp);
	else
		this->doReScan(target, needles, comp);
}

void Scanner::runDataStructureScan(const ScannerTargetShPtr &target)
{
	ASSERT(target.get() != nullptr);
	this->doDataStructureScan(target);
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
			if (meminfo.isCommitted)
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
	// this is just used for console output,
	// makes debugging easier
	size_t blocknum = 1;
	std::string blocksMessage = "";
	std::cout << "Scanning... Block ";

	// scan each memory block in chunks of chunkSize
	auto chunkSize = target->getChunkSize();
	uint8_t* buffer = new uint8_t[chunkSize];
	for (auto block = blocks.cbegin(); block != blocks.cend(); block++, blocknum++)
	{
		// update console
		{
			for (size_t i = 0; i < blocksMessage.length(); i++)
				std::cout << '\b';
			std::stringstream msg;
			msg << blocknum << " of " << blocks.size();
			blocksMessage = msg.str();
			std::cout << blocksMessage;
		}

		// scan block
		auto endAddress = block->allocationEnd;
		auto currentChunkAddress = block->allocationBase;
		auto currentChunkSize = std::min(chunkSize, block->allocationSize);

		while (true)
		{
			if (!target->readArray<uint8_t>(currentChunkAddress, currentChunkSize, buffer))
				break;

			callback(currentChunkAddress, buffer, chunkSize);

			currentChunkAddress = (MemoryAddress)((size_t)currentChunkAddress + currentChunkSize);
			if (currentChunkAddress >= endAddress)
				break;
			currentChunkSize = std::min(chunkSize, (size_t)endAddress - (size_t)currentChunkAddress);
		}
	}

	std::cout << std::endl;
	delete [] buffer;
}

void Scanner::doScan(const ScannerTargetShPtr &target, const ScanResultCollection &needles, const CompareTypeFlags &compType)
{
	// determine which blocks of memory can be scanned
	auto blocks = this->getScannableBlocks(target);

	// helper lambda that takes care of scanning each chunk
	ScanResultMap results;
	ScanResultAddressAllocator resLocAllocator;
	auto scanChunk = [needles, compType, &resLocAllocator, &results]
					(const MemoryAddress &baseAddress, const uint8_t* chunk, const size_t &chunkSize)
					-> void
	{
		// for each needle generated, see if it's in the chunk
		std::vector<size_t> locations;
		for (auto needle = needles.cbegin(); needle != needles.cend(); needle++)
		{
			locations.clear();
			needle->searchForMatchesInChunk(chunk, chunkSize, compType, baseAddress, locations);
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
				auto found = results.find(resultLoc);
				if (found == results.end())
				{
					ScanResultCollection temp;
					temp.push_back(ScanVariant(chunkSize - *loc, &chunk[*loc], *needle));
					results.emplace(std::make_pair(resultLoc, temp));
				}
				else
					found->second.push_back(*needle);
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
			auto res = needle->compareTo(buffer); 
			if ((res & compType) != 0)
				newResultValues.push_back(ScanVariant(bufferSize, buffer, *needle));
		}

		if (newResultValues.size())
			newResults.insert(std::make_pair(resultLocation->first, newResultValues));
	}
	delete [] buffer;
	this->scanState->updateState(newResults);
}

void Scanner::doDataStructureScan(const ScannerTargetShPtr &target)
{
	// determine which blocks of memory can be scanned
	auto blocks = this->getScannableBlocks(target);

	// calculate block bounds, this will help speed up the pointer locator
	MemoryAddress upperBound, lowerBound;
	this->calculateBoundsOfBlocks(target, blocks, lowerBound, upperBound);

	// first, we need to scan through every block and find any values which seem
	// to be valid pointers within the target. We use a map to dedupe, since,
	// even if a pointer appears multiple times, we only need to scan it once
	PointerMap foundPointers;
	auto findPointers = [this, upperBound, lowerBound, &target, &blocks, &foundPointers]
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
				foundPointers[check].push_back(location);
			}
		}
	};
	this->iterateOverBlocks(target, blocks, findPointers);

	// with the list of pointers, scan for valid structures
	DataStructureResultMap results;
	DataStructureBlueprint::findDataStructures(target, foundPointers, results);
	this->scanState->updateState(results);
}