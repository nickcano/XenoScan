tests.reset()

a = range(1, 100)
b = range(100, 1)
tests.assertEqual(a, b, "Ranges not automatically deducing min and max")


a = range(uint32, 100, 500)
b = uint32(range(100, 500))
tests.assertEqual(a, b, "type(range(a, b)) and range(type, a, b) not equal")