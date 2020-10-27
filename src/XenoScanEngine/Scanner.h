#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>

#include "ScannerTypes.h"

#include "ScannerTarget.h"
#include "ScanVariant.h"
#include "ScanResult.h"
#include "ScanState.h"
#include "RangeList.h"


class Scanner
{
public:
	typedef uint32_t ScanInferType;
	enum _ScanInferType : ScanInferType
	{
		SCAN_INFER_TYPE_ALL_TYPES,
		SCAN_INFER_TYPE_STRING_TYPES,
		SCAN_INFER_TYPE_NUMERIC_TYPES,
		SCAN_INFER_TYPE_EXACT,

		SCAN_INFER_TYPE_END = SCAN_INFER_TYPE_NUMERIC_TYPES
	};

	enum _CompareTypeFlags : CompareTypeFlags
	{
		SCAN_COMPARE_BEGIN = 1,

		// flags
		SCAN_COMPARE_EQUALS = SCAN_COMPARE_BEGIN,
		SCAN_COMPARE_GREATER_THAN = 2,
		SCAN_COMPARE_LESS_THAN = 4,

		// aggregates
		SCAN_COMPARE_GREATER_THAN_OR_EQUALS = 3,
		SCAN_COMPARE_LESS_THAN_OR_EQUALS = 5,

		// always (used for placholders in structures)
		SCAN_COMPARE_ALWAYS_MATCH = 128,

		SCAN_COMPARE_END = SCAN_COMPARE_LESS_THAN_OR_EQUALS
	};

	ScanStateShPtr scanState;

	Scanner();
	~Scanner();

	void startNewScan();
	void runScan(const ScannerTargetShPtr &target, const ScanVariant &needle, const CompareTypeFlags &comp, const ScanInferType &type);
	void runDataStructureScan(const ScannerTargetShPtr &target, const std::string &type);
	
private:
	typedef IRangeList<typename ScanVariant::ScanVariantType> IScanVariantTypeRange;
	typedef RangeList<typename ScanVariant::ScanVariantType> ScanVariantTypeRange;
	typedef RangeListAggregate<typename ScanVariant::ScanVariantType> ScanVariantTypeRangeAggregate;
	ScanVariantTypeRange inferCrosswalkStrings;
	ScanVariantTypeRange inferCrosswalkNumbers;
	ScanVariantTypeRangeAggregate inferCrosswalkAll;
	IScanVariantTypeRange* inferTypeCrosswalk[SCAN_INFER_TYPE_END + 1];

	MemoryInformationCollection getScannableBlocks(const ScannerTargetShPtr &target) const;

	typedef std::function<void(const MemoryAddress &baseAddress, const uint8_t* chunk, const size_t &chunkSize)> blockIterationCallback;
	void iterateOverBlocks(const ScannerTargetShPtr &target, const MemoryInformationCollection &blocks, blockIterationCallback callback) const;
	void calculateBoundsOfBlocks(const ScannerTargetShPtr &target, const MemoryInformationCollection &blocks, MemoryAddress &lower, MemoryAddress &upper) const;
	inline bool isValidPointer(const MemoryAddress &lower, const MemoryAddress &upper, const MemoryAddress &address) const
	{
		return (address >= lower && address <= upper);
	}

	void doScan(const ScannerTargetShPtr &target, const ScanResultCollection &needles, const CompareTypeFlags &compType);
	void doReScan(const ScannerTargetShPtr &target, const ScanResultCollection &needles, const CompareTypeFlags &compType);

	void doDataStructureScan(const ScannerTargetShPtr &target, const std::string &type);
};
typedef std::shared_ptr<Scanner> ScannerShPtr;