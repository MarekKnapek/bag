#include "check_ret.h"
#include "native.h"
#include "rosbag_print_info.h"
#include "scope_exit.h"

#include <cstdio> // std::puts
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS


bool bag__(int const argc, nchar const* const* const argv);
bool bag_(int const argc, nchar const* const* const argv);
bool bag(int const argc, nchar const* const* const argv);


int native_main(int const argc, nchar const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([&](){ std::puts("Oh, no! Someting went wrong!"); });

	bool const bussiness = bag__(argc, argv);
	CHECK_RET(bussiness, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash! Great Success!");
	return EXIT_SUCCESS;
}


bool bag__(int const argc, nchar const* const* const argv)
{
	MK_SEH_TRY
	{
		bool const bussiness = bag_(argc, argv);
		CHECK_RET_F(bussiness);
		return true;
	}
	MK_SEH_EXCEPT
	{
		return false;
	}
}

bool bag_(int const argc, nchar const* const* const argv)
{
	try
	{
		bool const bussiness = bag(argc, argv);
		CHECK_RET_F(bussiness);
		return true;
	}
	catch(...)
	{
		return false;
	}
}

bool bag(int const argc, nchar const* const* const argv)
{
	return mk::rosbag::print_info(argc, argv);
}
