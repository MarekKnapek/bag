#include "rosbag_header.h"

#include "rosbag_detail.h"
#include "check_ret.h"
#include "rosbag.h"

#include <cassert>
#include <cstring> // std::memcmp
#include <cstdint> // std::uint32_t
#include <algorithm> // std::all_of
#include <iterator> // std::size


namespace mk
{
	namespace rosbag
	{
		namespace detail
		{


			static constexpr char const s_field_op_name[] = "op";
			static constexpr int const s_field_op_name_len = static_cast<int>(std::size(s_field_op_name)) - 1;

			static constexpr char const s_field_name_and_data_separator = '=';

			static constexpr char const s_field_bag_index_pos_name[] = "index_pos";
			static constexpr int const s_field_bag_index_pos_name_len = static_cast<int>(std::size(s_field_bag_index_pos_name)) - 1;
			static constexpr char const s_field_bag_conn_count_name[] = "conn_count";
			static constexpr int const s_field_bag_conn_count_name_len = static_cast<int>(std::size(s_field_bag_conn_count_name)) - 1;
			static constexpr char const s_field_bag_chunk_count_name[] = "chunk_count";
			static constexpr int const s_field_bag_chunk_count_name_len = static_cast<int>(std::size(s_field_bag_chunk_count_name)) - 1;

			static constexpr char const s_field_chunk_compression_name[] = "compression";
			static constexpr int const s_field_chunk_compression_name_len = static_cast<int>(std::size(s_field_chunk_compression_name)) - 1;
			static constexpr char const s_field_chunk_size_name[] = "size";
			static constexpr int const s_field_chunk_size_name_len = static_cast<int>(std::size(s_field_chunk_size_name)) - 1;

			static constexpr char const s_field_connection_conn_name[] = "conn";
			static constexpr int const s_field_connection_conn_name_len = static_cast<int>(std::size(s_field_connection_conn_name)) - 1;
			static constexpr char const s_field_connection_topic_name[] = "topic";
			static constexpr int const s_field_connection_topic_name_len = static_cast<int>(std::size(s_field_connection_topic_name)) - 1;

			static constexpr char const s_field_message_data_conn_name[] = "conn";
			static constexpr int const s_field_message_data_conn_name_len = static_cast<int>(std::size(s_field_message_data_conn_name)) - 1;
			static constexpr char const s_field_message_data_time_name[] = "time";
			static constexpr int const s_field_message_data_time_name_len = static_cast<int>(std::size(s_field_message_data_time_name)) - 1;

			static constexpr char const s_field_index_data_ver_name[] = "ver";
			static constexpr int const s_field_index_data_ver_name_len = static_cast<int>(std::size(s_field_index_data_ver_name)) - 1;
			static constexpr char const s_field_index_data_conn_name[] = "conn";
			static constexpr int const s_field_index_data_conn_name_len = static_cast<int>(std::size(s_field_index_data_conn_name)) - 1;
			static constexpr char const s_field_index_data_count_name[] = "count";
			static constexpr int const s_field_index_data_count_name_len = static_cast<int>(std::size(s_field_index_data_count_name)) - 1;

			static constexpr char const s_field_chunk_info_ver_name[] = "ver";
			static constexpr int const s_field_chunk_info_ver_name_len = static_cast<int>(std::size(s_field_chunk_info_ver_name)) - 1;
			static constexpr char const s_field_chunk_info_chunk_pos_name[] = "chunk_pos";
			static constexpr int const s_field_chunk_info_chunk_pos_name_len = static_cast<int>(std::size(s_field_chunk_info_chunk_pos_name)) - 1;
			static constexpr char const s_field_chunk_info_start_time_name[] = "start_time";
			static constexpr int const s_field_chunk_info_start_time_name_len = static_cast<int>(std::size(s_field_chunk_info_start_time_name)) - 1;
			static constexpr char const s_field_chunk_info_end_time_name[] = "end_time";
			static constexpr int const s_field_chunk_info_end_time_name_len = static_cast<int>(std::size(s_field_chunk_info_end_time_name)) - 1;
			static constexpr char const s_field_chunk_info_count_name[] = "count";
			static constexpr int const s_field_chunk_info_count_name_len = static_cast<int>(std::size(s_field_chunk_info_count_name)) - 1;


			bool read_field(span_t& header_span, char const** const out_name_begin, char const** const out_name_end, unsigned char const** const out_data_begin, unsigned char const** const out_data_end);
			bool is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, int const target_name_len);


		}
	}
}


bool mk::rosbag::read_header_type(span_t file_span, bool* const out_op_found, header_type* const out_header_type)
{
	assert(out_op_found);
	assert(out_header_type);

	CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	CHECK_RET_F(header_len <= detail::s_large_value);
	CHECK_RET_F(header_len <= file_span.m_len);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len))
		{
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::bag) || op == static_cast<field_op_type>(header_type::chunk) || op == static_cast<field_op_type>(header_type::connection) || op == static_cast<field_op_type>(header_type::message_data) || op == static_cast<field_op_type>(header_type::index_data) || op == static_cast<field_op_type>(header_type::chunk_info));
			*out_op_found = true;
			*out_header_type = static_cast<header_type>(op);
			return true;
		}
		consume(header_span, field_data_len);
	}
	*out_op_found = false;
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_type const header_type, header_t* const out_header)
{
	assert(out_header);

	switch(header_type)
	{
		case header_type::bag:
		{
			bool const read = read_header(file_span, &out_header->m_bag);
			CHECK_RET_F(read);
		}
		break;
		case header_type::chunk:
		{
			bool const read = read_header(file_span, &out_header->m_chunk);
			CHECK_RET_F(read);
		}
		break;
		case header_type::connection:
		{
			bool const read = read_header(file_span, &out_header->m_connection);
			CHECK_RET_F(read);
		}
		break;
		case header_type::message_data:
		{
			bool const read = read_header(file_span, &out_header->m_message_data);
			CHECK_RET_F(read);
		}
		break;
		case header_type::index_data:
		{
			bool const read = read_header(file_span, &out_header->m_index_data);
			CHECK_RET_F(read);
		}
		break;
		case header_type::chunk_info:
		{
			bool const read = read_header(file_span, &out_header->m_chunk_info);
			CHECK_RET_F(read);
		}
		break;
	}

	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_bag_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_index_pos = false;
	bool read_conn_count = false;
	bool read_chunk_count = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_bag_index_pos_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_bag_index_pos_name, detail::s_field_bag_index_pos_name_len))
		{
			CHECK_RET_F(!read_index_pos);
			read_index_pos = true;
			out_header->m_index_pos = read<field_bag_index_pos_type>(header_span);
		}
		else if(field_data_len == sizeof(field_bag_conn_count_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_bag_conn_count_name, detail::s_field_bag_conn_count_name_len))
		{
			CHECK_RET_F(!read_conn_count);
			read_conn_count = true;
			out_header->m_conn_count = read<field_bag_conn_count_type>(header_span);
		}
		else if(field_data_len == sizeof(field_bag_chunk_count_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_bag_chunk_count_name, detail::s_field_bag_chunk_count_name_len))
		{
			CHECK_RET_F(!read_chunk_count);
			read_chunk_count = true;
			out_header->m_chunk_count = read<field_bag_chunk_count_type>(header_span);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::bag));
		}
	}
	CHECK_RET_F(read_index_pos && read_conn_count && read_chunk_count && read_op);
	consume(file_span, header_len);
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_chunk_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_compression = false;
	bool read_size = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_compression_name, detail::s_field_chunk_compression_name_len))
		{
			CHECK_RET_F(!read_compression);
			read_compression = true;
			int const str_len = static_cast<int>(field_data_end - field_data_begin);
			out_header->m_compression = string_t{reinterpret_cast<char const*>(field_data_begin), str_len};
			CHECK_RET_F(std::all_of(reinterpret_cast<char const*>(field_data_begin), reinterpret_cast<char const*>(field_data_end), detail::is_ascii));
			consume(header_span, str_len);
		}
		else if(field_data_len == sizeof(field_chunk_size_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_size_name, detail::s_field_chunk_size_name_len))
		{
			CHECK_RET_F(!read_size);
			read_size = true;
			out_header->m_size = read<field_chunk_size_type>(header_span);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::chunk));
		}
	}
	CHECK_RET_F(read_compression && read_size && read_op);
	consume(file_span, header_len);
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_connection_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_conn = false;
	bool read_topic = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_connection_conn_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_connection_conn_name, detail::s_field_connection_conn_name_len))
		{
			CHECK_RET_F(!read_conn);
			read_conn = true;
			out_header->m_conn = read<field_connection_conn_type>(header_span);
		}
		else if(detail::is_field_name(field_name_begin, field_name_end, detail::s_field_connection_topic_name, detail::s_field_connection_topic_name_len))
		{
			CHECK_RET_F(!read_topic);
			read_topic = true;
			int const str_len = static_cast<int>(field_data_end - field_data_begin);
			out_header->m_topic = string_t{reinterpret_cast<char const*>(field_data_begin), str_len};
			CHECK_RET_F(std::all_of(reinterpret_cast<char const*>(field_data_begin), reinterpret_cast<char const*>(field_data_end), detail::is_ascii));
			consume(header_span, str_len);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::connection));
		}
	}
	CHECK_RET_F(read_conn && read_topic && read_op);
	consume(file_span, header_len);
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_message_data_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_conn = false;
	bool read_time = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_message_data_conn_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_message_data_conn_name, detail::s_field_message_data_conn_name_len))
		{
			CHECK_RET_F(!read_conn);
			read_conn = true;
			out_header->m_conn = read<field_message_data_conn_type>(header_span);
		}
		else if(field_data_len == sizeof(field_message_data_time_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_message_data_time_name, detail::s_field_message_data_time_name_len))
		{
			CHECK_RET_F(!read_time);
			read_time = true;
			out_header->m_time = read<field_message_data_time_type>(header_span);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::message_data));
		}
	}
	CHECK_RET_F(read_conn && read_time && read_op);
	consume(file_span, header_len);
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_index_data_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_ver = false;
	bool read_conn = false;
	bool read_count = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_index_data_ver_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_index_data_ver_name, detail::s_field_index_data_ver_name_len))
		{
			CHECK_RET_F(!read_ver);
			read_ver = true;
			out_header->m_ver = read<field_index_data_ver_type>(header_span);
		}
		else if(field_data_len == sizeof(field_index_data_conn_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_index_data_conn_name, detail::s_field_index_data_conn_name_len))
		{
			CHECK_RET_F(!read_conn);
			read_conn = true;
			out_header->m_conn = read<field_index_data_conn_type>(header_span);
		}
		else if(field_data_len == sizeof(field_index_data_count_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_index_data_count_name, detail::s_field_index_data_count_name_len))
		{
			CHECK_RET_F(!read_count);
			read_count = true;
			out_header->m_count = read<field_index_data_count_type>(header_span);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::index_data));
		}
	}
	CHECK_RET_F(read_ver && read_conn && read_count && read_op);
	consume(file_span, header_len);
	return true;
}

bool mk::rosbag::read_header(span_t& file_span, header_chunk_info_t* const out_header)
{
	assert(out_header);
	std::uint32_t const header_len = read<std::uint32_t>(file_span);
	span_t header_span = span_t{file_span.m_ptr, header_len};
	bool read_ver = false;
	bool read_chunk_pos = false;
	bool read_start_time = false;
	bool read_end_time = false;
	bool read_count = false;
	bool read_op = false;
	while(header_span.m_len != 0)
	{
		char const* field_name_begin;
		char const* field_name_end;
		unsigned char const* field_data_begin;
		unsigned char const* field_data_end;
		bool const field_read = detail::read_field(header_span, &field_name_begin, &field_name_end, &field_data_begin, &field_data_end);
		CHECK_RET_F(field_read);
		std::uint32_t const field_data_len = static_cast<std::uint32_t>(field_data_end - field_data_begin);
		if(field_data_len == sizeof(field_chunk_info_ver_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_info_ver_name, detail::s_field_chunk_info_ver_name_len))
		{
			CHECK_RET_F(!read_ver);
			read_ver = true;
			out_header->m_ver = read<field_chunk_info_ver_type>(header_span);
		}
		else if(field_data_len == sizeof(field_chunk_info_chunk_pos_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_info_chunk_pos_name, detail::s_field_chunk_info_chunk_pos_name_len))
		{
			CHECK_RET_F(!read_chunk_pos);
			read_chunk_pos = true;
			out_header->m_chunk_pos = read<field_chunk_info_chunk_pos_type>(header_span);
		}
		else if(field_data_len == sizeof(field_chunk_info_start_time_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_info_start_time_name, detail::s_field_chunk_info_start_time_name_len))
		{
			CHECK_RET_F(!read_start_time);
			read_start_time = true;
			out_header->m_start_time = read<field_chunk_info_start_time_type>(header_span);
		}
		else if(field_data_len == sizeof(field_chunk_info_end_time_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_info_end_time_name, detail::s_field_chunk_info_end_time_name_len))
		{
			CHECK_RET_F(!read_end_time);
			read_end_time = true;
			out_header->m_end_time = read<field_chunk_info_end_time_type>(header_span);
		}
		else if(field_data_len == sizeof(field_chunk_info_count_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_chunk_info_count_name, detail::s_field_chunk_info_count_name_len))
		{
			CHECK_RET_F(!read_count);
			read_count = true;
			out_header->m_count = read<field_chunk_info_count_type>(header_span);
		}
		else
		{
			CHECK_RET_F(!read_op);
			read_op = true;
			CHECK_RET_F(field_data_len == sizeof(field_op_type) && detail::is_field_name(field_name_begin, field_name_end, detail::s_field_op_name, detail::s_field_op_name_len));
			field_op_type const op = read<field_op_type>(header_span);
			CHECK_RET_F(op == static_cast<field_op_type>(header_type::chunk_info));
		}
	}
	CHECK_RET_F(read_ver && read_chunk_pos && read_start_time && read_end_time && read_count && read_op);
	consume(file_span, header_len);
	return true;
}


bool mk::rosbag::detail::read_field(span_t& header_span, char const** const out_name_begin, char const** const out_name_end, unsigned char const** const out_data_begin, unsigned char const** const out_data_end)
{
	assert(out_name_begin);
	assert(out_name_end);
	assert(out_data_begin);
	assert(out_data_end);

	CHECK_RET_F(header_span.m_len >= sizeof(std::uint32_t));
	std::uint32_t const field_len = read<std::uint32_t>(header_span);
	CHECK_RET_F(field_len <= header_span.m_len);
	char const* const field_begin = static_cast<char const*>(header_span.m_ptr);
	char const* const field_end = field_begin + field_len;
	char const* field_name_end = std::find(field_begin, field_end, s_field_name_and_data_separator);
	CHECK_RET_F(field_name_end != field_end);
	CHECK_RET_F(field_name_end != field_begin);
	CHECK_RET_F(std::all_of(field_begin, field_name_end, detail::is_ascii));
	std::uint32_t const field_name_len = static_cast<std::uint32_t>(field_name_end - field_begin);
	consume(header_span, field_name_len + 1);
	std::uint32_t const field_data_len = field_len - field_name_len - 1;
	unsigned char const* const field_data_begin = static_cast<unsigned char const*>(header_span.m_ptr);
	unsigned char const* const field_data_end = field_data_begin + field_data_len;

	*out_name_begin = field_begin;
	*out_name_end = field_name_end;
	*out_data_begin = field_data_begin;
	*out_data_end = field_data_end;

	return true;
}

bool mk::rosbag::detail::is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, int const target_name_len)
{
	bool const is = field_name_end - field_name_begin == target_name_len && std::memcmp(field_name_begin, target_name_begin, target_name_len) == 0;
	return is;
}
