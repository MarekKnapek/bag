#include "rosbag_detail.h"


bool mk::rosbag::detail::is_ascii(char const ch)
{
	unsigned char const uch = static_cast<unsigned char>(ch);
	bool const is_valid = uch >= 0x20 && uch <= 0x7e;
	return is_valid;
}
