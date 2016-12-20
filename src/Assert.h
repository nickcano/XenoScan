#pragma once

#define ASSERT(x) do { \
if (!(x)) {	\
	__asm { int 3 } \
} \
} while (0, 0);