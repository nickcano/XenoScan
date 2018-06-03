tests.reset()

-- ranges tests
a = range(1, 100)
b = range(100, 1)
tests.assertEqual(a, b, "Ranges not automatically deducing min and max")


a = range(uint32, 100, 500)
b = uint32(range(100, 500))
tests.assertEqual(a, b, "type(range(a, b)) and range(type, a, b) not equal")

-- sizdeof tests
tests.assertEqual(
	sizeof(uint32("lol")),
	sizeof(uint32(1)),
	'sizeof(uint32("lol")) and sizeof(uint32(1)) are not equal'
)
tests.assertEqual(
	sizeof(range(uint32, 1, 5)),
	sizeof(uint32(range(1, 5))),
	'sizeof(range(uint32, 1, 5)) and sizeof(uint32(range(1, 5))) are not equal'
)
tests.assertEqual(
	sizeof(uint32),
	sizeof(uint32(1)),
	'sizeof(uint32) and sizeof(uint32(1)) are not equal'
)

-- sizeof/offsetof on structs
testStruct = struct(
	uint8("one"),
	int8("two"),
	uint16("three"),
	int16("four"),
	uint32("five"),
	int32("six"),
	uint64("seven"),
	int64("eight"),
	float("nine"),
	double("ten")
)
tests.assertEqual(offsetof(testStruct, "one"), 0, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "two"), 1, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "three"), 2, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "four"), 4, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "five"), 6, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "six"), 10, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "seven"), 14, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "eight"), 22, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "nine"), 30, "offsetof() failed!")
tests.assertEqual(offsetof(testStruct, "ten"), 34, "offsetof() failed!")
tests.assertEqual(sizeof(testStruct), 42, "sizeof() on struct failed!")