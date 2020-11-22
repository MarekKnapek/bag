#pragma once


#include <cstddef> // std::size_t


namespace mk
{
	namespace rosbag
	{

		struct span_t
		{
			void const* m_ptr;
			std::size_t m_len;
		};
		void consume(span_t& span, std::size_t const count);

		bool has_magic(span_t const& span);
		void consume_magic(span_t& span);

		void read(span_t& span, void* const destination, std::size_t const count);
		template<typename t> t read(span_t& span) { t val; read(span, &val, sizeof(t)); return val; }

	}
}
