#pragma once

#include "ScannerTypes.h"
#include "ScanVariant.h"
#include "ScanResult.h"
#include "ScannerDataStructureBlueprint.h"

class ScanState
{
public:
	ScanState() : firstScan(true), firstResults(), lastResults(), foundStructures() {}

	void clearScanResults()
	{
		this->firstScan = true;
		this->firstResults.clear();
		this->lastResults.clear();
	}

	inline bool isFirstScan() const { return this->firstScan; }
	void updateState(const ScanResultMap& results)
	{
		if (this->isFirstScan())
		{
			this->firstScan = false;
			this->firstResults = results;
			this->lastResults = this->firstResults;

			printf("%d initial matches found\n", this->lastResults.size());
		}
		else
		{
			auto oldSize = this->lastResults.size();
			this->lastResults = results;
			printf("Narrowed results from %d to %d\n", oldSize, this->lastResults.size());
		}

		// TODO: remove print
	}

	void updateState(ScanDataStructureResultMap& results)
	{
		this->foundStructures.swap(results);
		results.clear();

		// TODO: remove print
		/*for (auto found = this->foundStructures.cbegin(); found != this->foundStructures.cend(); found++)
		{
			for (auto object = found->second.cbegin(); object != found->second.cend(); object++)
			{
				if (object->second.objectCount > 1)
				{
					std::cout << "Found " << found->first << " at 0x"
						<< std::hex << object->second.rootNode
						<< " with " << std::dec << object->second.objectCount
						<< " entries" << std::endl;
				}
			}
		}*/
	}

	size_t resultSize() const { return this->lastResults.size(); }
	ScanResultMap::const_iterator beginResult() const { return this->lastResults.cbegin(); }
	ScanResultMap::const_iterator endResult() const { return this->lastResults.cend(); }
	const ScanDataStructureResultMap foundDataStructures() const { return foundStructures; }

private:
	bool firstScan;
	ScanResultMap firstResults, lastResults;
	ScanDataStructureResultMap foundStructures;
};
typedef std::shared_ptr<ScanState> ScanStateShPtr;