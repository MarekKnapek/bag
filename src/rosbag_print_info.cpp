#include "rosbag_print_info.h"

#include "check_ret.h"
#include "read_only_memory_mapped_file.h"
#include "rosbag.h"

#include <algorithm> // std::find
#include <cinttypes> // PRIu32, PRIu64
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t
#include <cstdio> // std::printf, std::puts
#include <iterator> // std::size


namespace mk
{
	namespace rosbag
	{
		namespace detail
		{
			static constexpr char const s_field_op_name[] = "op";
			static constexpr int const s_field_op_name_len = static_cast<int>(std::size(s_field_op_name)) - 1;
			typedef std::uint8_t field_op_type;

			typedef char const* string_t;

			static constexpr char const s_header_type_bag_name[] = "bag";
			static constexpr char const s_header_type_chunk_name[] = "chunk";
			static constexpr char const s_header_type_connection_name[] = "connection";
			static constexpr char const s_header_type_message_data_name[] = "message_data";
			static constexpr char const s_header_type_index_data_name[] = "index_data";
			static constexpr char const s_header_type_chunk_info_name[] = "chunk_info";
			static constexpr char const s_header_type_unknown_name[] = "unknown";

			static constexpr char const s_field_bag_index_pos_name[] = "index_pos";
			static constexpr int const s_field_bag_index_pos_name_len = static_cast<int>(std::size(s_field_bag_index_pos_name)) - 1;
			typedef std::uint64_t field_bag_index_pos_type;
			#define field_bag_index_pos_type_specifier PRIu64
			static constexpr char const s_field_bag_conn_count_name[] = "conn_count";
			static constexpr int const s_field_bag_conn_count_name_len = static_cast<int>(std::size(s_field_bag_conn_count_name)) - 1;
			typedef std::uint32_t field_bag_conn_count_type;
			#define field_bag_conn_count_type_specifier PRIu32
			static constexpr char const s_field_bag_chunk_count_name[] = "chunk_count";
			static constexpr int const s_field_bag_chunk_count_name_len = static_cast<int>(std::size(s_field_bag_chunk_count_name)) - 1;
			typedef std::uint32_t field_bag_chunk_count_type;
			#define field_bag_chunk_count_type_specifier PRIu32

			static constexpr char const s_field_chunk_compression_name[] = "compression";
			static constexpr int const s_field_chunk_compression_name_len = static_cast<int>(std::size(s_field_chunk_compression_name)) - 1;
			typedef string_t field_chunk_compression_type;
			#define field_chunk_compression_type_specifier ".*s"
			static constexpr char const s_field_chunk_size_name[] = "size";
			static constexpr int const s_field_chunk_size_name_len = static_cast<int>(std::size(s_field_chunk_size_name)) - 1;
			typedef std::uint32_t field_chunk_size_type;
			#define field_chunk_size_type_specifier PRIu32

			static constexpr char const s_field_connection_conn_name[] = "conn";
			static constexpr int const s_field_connection_conn_name_len = static_cast<int>(std::size(s_field_connection_conn_name)) - 1;
			typedef std::uint32_t field_connection_conn_type;
			#define field_connection_conn_type_specifier PRIu32
			static constexpr char const s_field_connection_topic_name[] = "topic";
			static constexpr int const s_field_connection_topic_name_len = static_cast<int>(std::size(s_field_connection_topic_name)) - 1;
			typedef string_t field_connection_topic_type;
			#define field_connection_topic_type_specifier ".*s"

			static constexpr char const s_field_message_data_conn_name[] = "conn";
			static constexpr int const s_field_message_data_conn_name_len = static_cast<int>(std::size(s_field_message_data_conn_name)) - 1;
			typedef std::uint32_t field_message_data_conn_type;
			#define field_message_data_conn_type_specifier PRIu32
			static constexpr char const s_field_message_data_time_name[] = "time";
			static constexpr int const s_field_message_data_time_name_len = static_cast<int>(std::size(s_field_message_data_time_name)) - 1;
			typedef std::uint64_t field_message_data_time_type;
			#define field_message_data_time_type_specifier PRIu64

			static constexpr char const s_field_index_data_ver_name[] = "ver";
			static constexpr int const s_field_index_data_ver_name_len = static_cast<int>(std::size(s_field_index_data_ver_name)) - 1;
			typedef std::uint32_t field_index_data_ver_type;
			#define field_index_data_ver_type_specifier PRIu32
			static constexpr char const s_field_index_data_conn_name[] = "conn";
			static constexpr int const s_field_index_data_conn_name_len = static_cast<int>(std::size(s_field_index_data_conn_name)) - 1;
			typedef std::uint32_t field_index_data_conn_type;
			#define field_index_data_conn_type_specifier PRIu32
			static constexpr char const s_field_index_data_count_name[] = "count";
			static constexpr int const s_field_index_data_count_name_len = static_cast<int>(std::size(s_field_index_data_count_name)) - 1;
			typedef std::uint32_t field_index_data_count_type;
			#define field_index_data_count_type_specifier PRIu32

			static constexpr char const s_field_chunk_info_ver_name[] = "ver";
			static constexpr int const s_field_chunk_info_ver_name_len = static_cast<int>(std::size(s_field_chunk_info_ver_name)) - 1;
			typedef std::uint32_t field_chunk_info_ver_type;
			#define field_chunk_info_ver_type_specifier PRIu32
			static constexpr char const s_field_chunk_info_chunk_pos_name[] = "chunk_pos";
			static constexpr int const s_field_chunk_info_chunk_pos_name_len = static_cast<int>(std::size(s_field_chunk_info_chunk_pos_name)) - 1;
			typedef std::uint64_t field_chunk_info_chunk_pos_type;
			#define field_chunk_info_chunk_pos_type_specifier PRIu64
			static constexpr char const s_field_chunk_info_start_time_name[] = "start_time";
			static constexpr int const s_field_chunk_info_start_time_name_len = static_cast<int>(std::size(s_field_chunk_info_start_time_name)) - 1;
			typedef std::uint64_t field_chunk_info_start_time_type;
			#define field_chunk_info_start_time_type_specifier PRIu64
			static constexpr char const s_field_chunk_info_end_time_name[] = "end_time";
			static constexpr int const s_field_chunk_info_end_time_name_len = static_cast<int>(std::size(s_field_chunk_info_end_time_name)) - 1;
			typedef std::uint64_t field_chunk_info_end_time_type;
			#define field_chunk_info_end_time_type_specifier PRIu64
			static constexpr char const s_field_chunk_info_count_name[] = "count";
			static constexpr int const s_field_chunk_info_count_name_len = static_cast<int>(std::size(s_field_chunk_info_count_name)) - 1;
			typedef std::uint32_t field_chunk_info_count_type;
			#define field_chunk_info_count_type_specifier PRIu32

			enum class op_code : field_op_type
			{
				bag = 0x03,
				chunk = 0x05,
				connection = 0x07,
				message_data = 0x02,
				index_data = 0x04,
				chunk_info = 0x06,
			};

			bool is_ascii(char const ch);
			bool is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, char const* const target_name_end);
			bool is_field_op_name(char const* const field_name_begin, char const* const field_name_end);

			bool is_field_bag_index_pos_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_bag_conn_count_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_bag_chunk_count_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_compression_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_size_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_connection_conn_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_connection_topic_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_message_data_conn_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_message_data_time_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_index_data_ver_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_index_data_conn_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_index_data_count_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_info_ver_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_info_chunk_pos_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_info_start_time_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_info_end_time_name(char const* const field_name_begin, char const* const field_name_end);
			bool is_field_chunk_info_count_name(char const* const field_name_begin, char const* const field_name_end);

			bool is_valid_op(field_op_type const op);
			char const* op_to_string(field_op_type const op);
		}
	}
}


bool mk::rosbag::print_info(int const argc, nchar const* const* const argv)
{
	CHECK_RET_F(argc == 2);
	mk::read_only_memory_mapped_file_t const rommf{argv[1]};
	CHECK_RET_F(rommf);
	span_t span{rommf.get_data(), rommf.get_size()};
	CHECK_RET_F(has_magic(span));
	consume_magic(span);

	int record_idx = 0;
	while(span.m_len != 0)
	{
		CHECK_RET_F(span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const header_len = read<std::uint32_t>(span);
		CHECK_RET_F(header_len <= span.m_len);
		span_t header = span_t{span.m_ptr, header_len};
		detail::field_op_type op{};
		bool op_found = false;
		while(header.m_len != 0)
		{
			CHECK_RET_F(header.m_len >= sizeof(std::uint32_t));
			std::uint32_t const field_len = read<std::uint32_t>(header);
			CHECK_RET_F(field_len <= header.m_len);
			char const* const field_begin = static_cast<char const*>(header.m_ptr);
			char const* const field_end = field_begin + field_len;
			auto const field_name_end = std::find(field_begin, field_end, '=');
			CHECK_RET_F(field_name_end != field_end);
			CHECK_RET_F(field_name_end != field_begin);
			CHECK_RET_F(std::all_of(field_begin, field_name_end, detail::is_ascii));
			std::uint32_t const field_name_len = static_cast<std::uint32_t>(field_name_end - field_begin);
			consume(header, field_name_len + 1);
			std::uint32_t const field_data_len = field_len - field_name_len - 1;
			if(detail::is_field_op_name(field_begin, field_name_end))
			{
				CHECK_RET_F(field_data_len == sizeof(detail::field_op_type));
				op = read<detail::field_op_type>(header);
				op_found = true;
				break;
			}
			consume(header, field_data_len);
		}
		CHECK_RET_F(op_found);
		std::printf("Record #%d, type = %s", record_idx, detail::op_to_string(op));

		header = span_t{span.m_ptr, header_len};
		while(header.m_len != 0)
		{
			CHECK_RET_F(header.m_len >= sizeof(std::uint32_t));
			std::uint32_t const field_len = read<std::uint32_t>(header);
			CHECK_RET_F(field_len <= header.m_len);
			char const* const field_begin = static_cast<char const*>(header.m_ptr);
			char const* const field_end = field_begin + field_len;
			auto const field_name_end = std::find(field_begin, field_end, '=');
			CHECK_RET_F(field_name_end != field_end);
			CHECK_RET_F(field_name_end != field_begin);
			CHECK_RET_F(std::all_of(field_begin, field_name_end, detail::is_ascii));
			std::uint32_t const field_name_len = static_cast<std::uint32_t>(field_name_end - field_begin);
			consume(header, field_name_len + 1);
			std::uint32_t const field_data_len = field_len - field_name_len - 1;
			if(detail::is_field_op_name(field_begin, field_name_end))
			{
				consume(header, field_data_len);
				continue;
			}
			switch(op)
			{
				case static_cast<std::uint8_t>(detail::op_code::bag):
				{
					if(detail::is_field_bag_index_pos_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_index_pos_type));
						detail::field_bag_index_pos_type const val = read<detail::field_bag_index_pos_type>(header);
						std::printf(", %s = %" field_bag_index_pos_type_specifier "", detail::s_field_bag_index_pos_name, val);
					}
					else if(detail::is_field_bag_conn_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_conn_count_type));
						detail::field_bag_conn_count_type const val = read<detail::field_bag_conn_count_type>(header);
						std::printf(", %s = %" field_bag_conn_count_type_specifier "", detail::s_field_bag_conn_count_name, val);
					}
					else if(detail::is_field_bag_chunk_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_chunk_count_type));
						detail::field_bag_chunk_count_type const val = read<detail::field_bag_chunk_count_type>(header);
						std::printf(", %s = %" field_bag_chunk_count_type_specifier "", detail::s_field_bag_chunk_count_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::chunk):
				{
					if(detail::is_field_chunk_compression_name(field_begin, field_name_end))
					{
						detail::field_chunk_compression_type const val = field_name_end + 1;
						CHECK_RET_F(std::all_of(val, val + field_data_len, detail::is_ascii));
						std::printf(", %s = %" field_chunk_compression_type_specifier "", detail::s_field_chunk_compression_name, static_cast<int>(field_data_len), val);
						consume(header, field_data_len);
					}
					else if(detail::is_field_chunk_size_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_size_type));
						detail::field_chunk_size_type const val = read<detail::field_chunk_size_type>(header);
						std::printf(", %s = %" field_chunk_size_type_specifier "", detail::s_field_chunk_size_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::connection):
				{
					if(detail::is_field_connection_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_connection_conn_type));
						detail::field_connection_conn_type const val = read<detail::field_connection_conn_type>(header);
						std::printf(", %s = %" field_connection_conn_type_specifier "", detail::s_field_connection_conn_name, val);
					}
					else if(detail::is_field_connection_topic_name(field_begin, field_name_end))
					{
						detail::field_connection_topic_type const val = field_name_end + 1;
						CHECK_RET_F(std::all_of(val, val + field_data_len, detail::is_ascii));
						std::printf(", %s = %" field_connection_topic_type_specifier "", detail::s_field_connection_topic_name, static_cast<int>(field_data_len), val);
						consume(header, field_data_len);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::message_data):
				{
					if(detail::is_field_message_data_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_message_data_conn_type));
						detail::field_message_data_conn_type const val = read<detail::field_message_data_conn_type>(header);
						std::printf(", %s = %" field_message_data_conn_type_specifier "", detail::s_field_message_data_conn_name, val);
					}
					else if(detail::is_field_message_data_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_message_data_time_type));
						detail::field_message_data_time_type const val = read<detail::field_message_data_time_type>(header);
						std::printf(", %s = %" field_message_data_time_type_specifier "", detail::s_field_message_data_time_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::index_data):
				{
					if(detail::is_field_index_data_ver_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_ver_type));
						detail::field_index_data_ver_type const val = read<detail::field_index_data_ver_type>(header);
						std::printf(", %s = %" field_index_data_ver_type_specifier "", detail::s_field_index_data_ver_name, val);
					}
					else if(detail::is_field_index_data_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_conn_type));
						detail::field_index_data_conn_type const val = read<detail::field_index_data_conn_type>(header);
						std::printf(", %s = %" field_index_data_conn_type_specifier "", detail::s_field_index_data_conn_name, val);
					}
					else if(detail::is_field_index_data_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_count_type));
						detail::field_index_data_count_type const val = read<detail::field_index_data_count_type>(header);
						std::printf(", %s = %" field_index_data_count_type_specifier "", detail::s_field_index_data_count_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::chunk_info):
				{
					if(detail::is_field_chunk_info_ver_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_ver_type));
						detail::field_chunk_info_ver_type const val = read<detail::field_chunk_info_ver_type>(header);
						std::printf(", %s = %" field_chunk_info_ver_type_specifier "", detail::s_field_chunk_info_ver_name, val);
					}
					else if(detail::is_field_chunk_info_chunk_pos_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_chunk_pos_type));
						detail::field_chunk_info_chunk_pos_type const val = read<detail::field_chunk_info_chunk_pos_type>(header);
						std::printf(", %s = %" field_chunk_info_chunk_pos_type_specifier "", detail::s_field_chunk_info_chunk_pos_name, val);
					}
					else if(detail::is_field_chunk_info_start_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_start_time_type));
						detail::field_chunk_info_start_time_type const val = read<detail::field_chunk_info_start_time_type>(header);
						std::printf(", %s = %" field_chunk_info_start_time_type_specifier "", detail::s_field_chunk_info_start_time_name, val);
					}
					else if(detail::is_field_chunk_info_end_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_end_time_type));
						detail::field_chunk_info_end_time_type const val = read<detail::field_chunk_info_end_time_type>(header);
						std::printf(", %s = %" field_chunk_info_end_time_type_specifier "", detail::s_field_chunk_info_end_time_name, val);
					}
					else if(detail::is_field_chunk_info_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_count_type));
						detail::field_chunk_info_count_type const val = read<detail::field_chunk_info_count_type>(header);
						std::printf(", %s = %" field_chunk_info_count_type_specifier "", detail::s_field_chunk_info_count_name, val);
					}
				}
				break;
				default:
				{
					std::printf(", %.*s len %" PRIu32 "", static_cast<int>(field_name_len), field_begin, field_data_len);
					consume(header, field_data_len);
				}
				break;
			}
		}
		consume(span, header_len);

		CHECK_RET_F(span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const record_data_len = read<std::uint32_t>(span);
		CHECK_RET_F(record_data_len <= span.m_len);
		std::printf(", record data len = %" PRIu32 "", record_data_len);

		consume(span, record_data_len);
		std::printf("%s", "\n");
		++record_idx;
	}

	return true;
}


bool mk::rosbag::detail::is_ascii(char const ch)
{
	unsigned char const uch = static_cast<unsigned char>(ch);
	bool const is_valid = uch >= 0x20 && uch <= 0x7e;
	return is_valid;
}

bool mk::rosbag::detail::is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, char const* const target_name_end)
{
	bool const is = field_name_end - field_name_begin == target_name_end - target_name_begin && std::memcmp(field_name_begin, target_name_begin, target_name_end - target_name_begin) == 0;
	return is;
}

bool mk::rosbag::detail::is_field_op_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_op_name, s_field_op_name + s_field_op_name_len);
	return is;
}

bool mk::rosbag::detail::is_valid_op(field_op_type const op)
{
	bool const is =
		op == static_cast<field_op_type>(op_code::bag) ||
		op == static_cast<field_op_type>(op_code::chunk) ||
		op == static_cast<field_op_type>(op_code::connection) ||
		op == static_cast<field_op_type>(op_code::message_data) ||
		op == static_cast<field_op_type>(op_code::index_data) ||
		op == static_cast<field_op_type>(op_code::chunk_info);
	return is;
}

char const* mk::rosbag::detail::op_to_string(field_op_type const op)
{
	char const* str;
	switch(op)
	{
		case static_cast<std::uint8_t>(op_code::bag):
		{
			str = s_header_type_bag_name;
		}
		break;
		case static_cast<std::uint8_t>(op_code::chunk):
		{
			str = s_header_type_chunk_name;
		}
		break;
		case static_cast<std::uint8_t>(op_code::connection):
		{
			str = s_header_type_connection_name;
		}
		break;
		case static_cast<std::uint8_t>(op_code::message_data):
		{
			str = s_header_type_message_data_name;
		}
		break;
		case static_cast<std::uint8_t>(op_code::index_data):
		{
			str = s_header_type_index_data_name;
		}
		break;
		case static_cast<std::uint8_t>(op_code::chunk_info):
		{
			str = s_header_type_chunk_info_name;
		}
		break;
		default:
		{
			str = s_header_type_unknown_name;
		}
		break;
	}
	return str;
}


bool mk::rosbag::detail::is_field_bag_index_pos_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_index_pos_name, s_field_bag_index_pos_name + s_field_bag_index_pos_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_bag_conn_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_conn_count_name, s_field_bag_conn_count_name + s_field_bag_conn_count_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_bag_chunk_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_chunk_count_name, s_field_bag_chunk_count_name + s_field_bag_chunk_count_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_compression_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_compression_name, s_field_chunk_compression_name + s_field_chunk_compression_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_size_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_size_name, s_field_chunk_size_name + s_field_chunk_size_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_connection_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_connection_conn_name, s_field_connection_conn_name + s_field_connection_conn_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_connection_topic_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_connection_topic_name, s_field_connection_topic_name + s_field_connection_topic_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_message_data_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_message_data_conn_name, s_field_message_data_conn_name + s_field_message_data_conn_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_message_data_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_message_data_time_name, s_field_message_data_time_name + s_field_message_data_time_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_index_data_ver_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_ver_name, s_field_index_data_ver_name + s_field_index_data_ver_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_index_data_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_conn_name, s_field_index_data_conn_name + s_field_index_data_conn_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_index_data_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_count_name, s_field_index_data_count_name + s_field_index_data_count_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_info_ver_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_ver_name, s_field_chunk_info_ver_name + s_field_chunk_info_ver_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_info_chunk_pos_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_chunk_pos_name, s_field_chunk_info_chunk_pos_name + s_field_chunk_info_chunk_pos_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_info_start_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_start_time_name, s_field_chunk_info_start_time_name + s_field_chunk_info_start_time_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_info_end_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_end_time_name, s_field_chunk_info_end_time_name + s_field_chunk_info_end_time_name_len);
	return is;
}

bool mk::rosbag::detail::is_field_chunk_info_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_count_name, s_field_chunk_info_count_name + s_field_chunk_info_count_name_len);
	return is;
}
