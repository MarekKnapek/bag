#define FUZZING


#include "check_ret.cpp"
#include "read_only_memory_mapped_file.cpp"
#include "read_only_memory_mapped_file_linux.cpp"
#include "rosbag.cpp"
#include "rosbag_print_info.cpp"


static unsigned g_do_not_optimise;


extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* const data, std::size_t const size)
{
	mk::rosbag::span_t file_span{data, size};
	bool const printed = mk::rosbag::detail::print_info(file_span);
	g_do_not_optimise += printed ? 1 : 0;
	return 0;
}


// clang++-10 -g -O1 -std=c++20 -DNDEBUG -fsanitize=fuzzer,address src/fuzz.cpp -o bag_fuzz.bin
// ./bag_fuzz.bin -dict=./fuzz_dict.txt ./fuzz_corpus
// ./bag_fuzz.bin -dict=./fuzz_dict.txt ./fuzz_corpus -jobs=4 -workers=4
