dofile("lib.lua")

TEST_STATUS = true
TEST_MESSAGES = {}

tests = {}

tests.reset = function ()
	TEST_STATUS = true
	TEST_MESSAGES = {}
end

tests.assertEqual = function(a, b, messgae)
	local ttest = function(_a, _b)
		return table.show(_a, "", "") == table.show(_b, "", "")
	end
	tests.assert(ttest(a, b), messgae)
end

tests.assertNotNil = function(a, messgae)
	tests.assert(a ~= nil, messgae)
end

tests.assert = function(value, message)
	TEST_STATUS = value
	if (not TEST_STATUS) then
		TEST_MESSAGES[#TEST_MESSAGES + 1] = message
	end
end
