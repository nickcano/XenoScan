tests.reset()

--------------- TEST STRINGS ---------------
function findStringResults(totype, value, expected)
	local proc = Process(TEST_PID)
	proc:newScan()
	proc:scanFor(totype(value))
	local results = proc:getResults()
	proc:destroy()

	return results[expected]
end

string1 = findStringResults(ascii, TEST_STRING1, TEST_STRING1_ADDRESS)
tests.assertNotNil(string1, "Failed to locate char[32]!")

string2 = findStringResults(ascii, TEST_STRING2, TEST_STRING2_ADDRESS)
tests.assertNotNil(string2, "Failed to locate std::string!")

string3 = findStringResults(widestring, TEST_STRING3, TEST_STRING3_ADDRESS)
tests.assertNotNil(string3, "Failed to locate std::wstring!")

--------------- TEST STRUCTURE (COMMON) ---------------
testStruct = struct(
	uint32("one"),
	uint32("two"),
	uint32("three"),
	uint32("four"),
	uint32("five"),
	uint32("six"),
	uint32("seven")
)

function findStructureResults()
	local proc = Process(TEST_PID)
	proc:newScan()
	proc:scanFor(testStruct)
	local results = proc:getResults()
	proc:destroy()

	return results[TEST_STRUCT_ADDRESS]
end

--------------- TEST STRUCTURE (STATIC) ---------------
testStruct["one"] = TEST_STRUCT_ONE
testStruct["two"] = TEST_STRUCT_TWO
testStruct["three"] = TEST_STRUCT_THREE
testStruct["four"] = TEST_STRUCT_FOUR
testStruct["five"] = TEST_STRUCT_FIVE
testStruct["six"] = TEST_STRUCT_SIX
testStruct["seven"] = TEST_STRUCT_SEVEN

tests.assertNotNil(findStructureResults(), "Failed to locate test structure (static)!")

--------------- TEST STRUCTURE (RANGES) ---------------
testStruct["one"] = range(TEST_STRUCT_ONE - 5, TEST_STRUCT_ONE + 5)
testStruct["two"] = TEST_STRUCT_TWO
testStruct["three"] = TEST_STRUCT_THREE
testStruct["four"] = {}
testStruct["five"] = TEST_STRUCT_FIVE
testStruct["six"] = {}
testStruct["seven"] = range(TEST_STRUCT_SEVEN - 1, TEST_STRUCT_SEVEN + 1)

tests.assertNotNil(findStructureResults(), "Failed to locate test structure (ranges)!")