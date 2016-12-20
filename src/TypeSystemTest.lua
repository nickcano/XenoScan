dofile("lib.lua")

function ttest(_a, _b)
	return table.show(_a, "", "") == table.show(_b, "", "")
end

tests = {}

a = range(1, 100)
b = range(100, 1)
assert(ttest(a, b), "Ranges not automatically deducing mion and max")

a = range(uint32, 100, 500)
b = uint32(range(100, 500))
assert(ttest(a, b), "type(range(a, b)) and range(type, a, b) not equal")

-- figure how to test this
tests[5] = struct(
	uint32("first"),
	uint32("second"),
	int8("third")
)
tests[5].first = range(1, 100)
tests[5].second = 1
tests[5].third = range(1, 100)
