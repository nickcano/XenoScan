tests.reset()

--------------- TEST STRINGS ---------------
function findStringResults(value)
	local proc = Process(TEST_PID)
	proc:newScan()
	proc:scanFor(value)
	local results = proc:getResults()
	proc:destroy()

	return #results >= 2
end

tests.assert(findStringResults(ascii(TEST_STRING1)), "Failed to locate char[32]!")
tests.assert(findStringResults(ascii(TEST_STRING2)), "Failed to locate std::string!")
tests.assert(findStringResults(widestring(TEST_STRING3)), "Failed to locate std::wstring!")


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

	local addressToFind = string.sub(tostring(TEST_STRUCT_ADDRESS), 11)
	for key, value in pairs(results) do
		if (string.ends(key, addressToFind)) then
			return true
		end
	end
	return false
end

--------------- TEST STRUCTURE (STATIC) ---------------
testStruct["one"] = TEST_STRUCT_ONE
testStruct["two"] = TEST_STRUCT_TWO
testStruct["three"] = TEST_STRUCT_THREE
testStruct["four"] = TEST_STRUCT_FOUR
testStruct["five"] = TEST_STRUCT_FIVE
testStruct["six"] = TEST_STRUCT_SIX
testStruct["seven"] = TEST_STRUCT_SEVEN

tests.assert(findStructureResults(), "Failed to locate test structure (static)!")

--------------- TEST STRUCTURE (RANGES) ---------------
testStruct["one"] = range(TEST_STRUCT_ONE - 5, TEST_STRUCT_ONE + 5)
testStruct["two"] = TEST_STRUCT_TWO
testStruct["three"] = TEST_STRUCT_THREE
testStruct["four"] = {}
testStruct["five"] = TEST_STRUCT_FIVE
testStruct["six"] = {}
testStruct["seven"] = range(TEST_STRUCT_SEVEN - 1, TEST_STRUCT_SEVEN + 1)

tests.assert(findStructureResults(), "Failed to locate test structure (ranges)!")