#include "rosbag.h"

#include "rosbag_detail.h"

#include <cassert>
#include <cstring> // std::memcmp, std::memcpy
#include <iterator> // std::size


void mk::rosbag::consume(span_t& span, std::size_t const count)
{
	assert(count <= span.m_len);
	span.m_ptr = static_cast<void const*>(static_cast<unsigned char const*>(span.m_ptr) + count);
	span.m_len = span.m_len - count;
}


bool mk::rosbag::has_magic(span_t const& span)
{
	if(detail::s_bag_magic_len > span.m_len)
	{
		return false;
	}
	bool const has = std::memcmp(span.m_ptr, detail::s_bag_magic, detail::s_bag_magic_len) == 0;
	return has;
}

void mk::rosbag::consume_magic(span_t& span)
{
	consume(span, detail::s_bag_magic_len);
}


void mk::rosbag::read(span_t& span, void* const destination, std::size_t const count)
{
	assert(count <= span.m_len);
	std::memcpy(destination, span.m_ptr, count);
	consume(span, count);
}
