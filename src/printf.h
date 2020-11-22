#pragma once


#ifdef FUZZING

	#define mk_printf(...)

#else

	#define mk_printf(...) std::printf(__VA_ARGS__)

#endif
