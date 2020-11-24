#pragma once


#include <iterator> // std::size


namespace mk
{
	namespace rosbag
	{
		namespace detail
		{


			static constexpr char const s_bag_magic[] = "#ROSBAG V2.0\x0A";
			static constexpr int const s_bag_magic_len = static_cast<int>(std::size(s_bag_magic)) - 1;

			static constexpr int const s_large_value = 2ull * 1024ull * 1024ull * 1024ull - 64ull * 1024ull; // 2 GB - 64 kB

			
			bool is_ascii(char const ch);


		}
	}
}
