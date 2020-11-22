#include "check_ret.h"

#include <array>
#include <cassert>
#include <cstdio> // std::snprintf, std::puts


#ifdef FUZZING

void mk::check_ret_failed(char const* const, int const, char const* const)
{
}

#else

void mk::check_ret_failed(char const* const file, int const line, char const* const expr)
{
	std::array<char, 1 * 1024> buff;
	int const formatted = std::snprintf(buff.data(), buff.size(), "CHECK_RET failed in file '%s' on line %d with '%s'.", file, line, expr);
	assert(formatted >= 0 && formatted < static_cast<int>(buff.size())); (void)formatted;
	int const printed = std::puts(buff.data());
	assert(printed != EOF && printed >= 0); (void)printed;
}

#endif
