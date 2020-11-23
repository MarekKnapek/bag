#include "rosbag_print_info.h"

#include "check_ret.h"
#include "printf.h"
#include "read_only_memory_mapped_file.h"
#include "rosbag.h"

#include <algorithm> // std::find
#include <cinttypes> // PRIu32, PRIu64
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t
#include <cstdio> // std::printf, std::puts
#include <cstring> // std::memset
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

			struct string_t
			{
				char const* m_begin;
				int m_len;
			};

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
			static constexpr unsigned const s_field_bag_index_pos_position_idx = 1u << 0;
			static constexpr char const s_field_bag_conn_count_name[] = "conn_count";
			static constexpr int const s_field_bag_conn_count_name_len = static_cast<int>(std::size(s_field_bag_conn_count_name)) - 1;
			typedef std::uint32_t field_bag_conn_count_type;
			#define field_bag_conn_count_type_specifier PRIu32
			static constexpr unsigned const s_field_bag_conn_count_position_idx = 1u << 1;
			static constexpr char const s_field_bag_chunk_count_name[] = "chunk_count";
			static constexpr int const s_field_bag_chunk_count_name_len = static_cast<int>(std::size(s_field_bag_chunk_count_name)) - 1;
			typedef std::uint32_t field_bag_chunk_count_type;
			#define field_bag_chunk_count_type_specifier PRIu32
			static constexpr unsigned const s_field_bag_chunk_count_position_idx = 1u << 2;

			static constexpr char const s_field_chunk_compression_name[] = "compression";
			static constexpr int const s_field_chunk_compression_name_len = static_cast<int>(std::size(s_field_chunk_compression_name)) - 1;
			typedef string_t field_chunk_compression_type;
			#define field_chunk_compression_type_specifier ".*s"
			static constexpr unsigned const s_field_chunk_compression_position_idx = 1u << 0;
			static constexpr char const s_field_chunk_size_name[] = "size";
			static constexpr int const s_field_chunk_size_name_len = static_cast<int>(std::size(s_field_chunk_size_name)) - 1;
			typedef std::uint32_t field_chunk_size_type;
			#define field_chunk_size_type_specifier PRIu32
			static constexpr unsigned const s_field_chunk_size_position_idx = 1u << 1;

			static constexpr char const s_field_connection_conn_name[] = "conn";
			static constexpr int const s_field_connection_conn_name_len = static_cast<int>(std::size(s_field_connection_conn_name)) - 1;
			typedef std::uint32_t field_connection_conn_type;
			#define field_connection_conn_type_specifier PRIu32
			static constexpr unsigned const s_field_connection_conn_position_idx = 1u << 0;
			static constexpr char const s_field_connection_topic_name[] = "topic";
			static constexpr int const s_field_connection_topic_name_len = static_cast<int>(std::size(s_field_connection_topic_name)) - 1;
			typedef string_t field_connection_topic_type;
			#define field_connection_topic_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_topic_position_idx = 1u << 1;

			static constexpr char const s_field_message_data_conn_name[] = "conn";
			static constexpr int const s_field_message_data_conn_name_len = static_cast<int>(std::size(s_field_message_data_conn_name)) - 1;
			typedef std::uint32_t field_message_data_conn_type;
			#define field_message_data_conn_type_specifier PRIu32
			static constexpr unsigned const s_field_message_data_conn_position_idx = 1u << 0;
			static constexpr char const s_field_message_data_time_name[] = "time";
			static constexpr int const s_field_message_data_time_name_len = static_cast<int>(std::size(s_field_message_data_time_name)) - 1;
			typedef std::uint64_t field_message_data_time_type;
			#define field_message_data_time_type_specifier PRIu64
			static constexpr unsigned const s_field_message_data_time_position_idx = 1u << 1;

			static constexpr char const s_field_index_data_ver_name[] = "ver";
			static constexpr int const s_field_index_data_ver_name_len = static_cast<int>(std::size(s_field_index_data_ver_name)) - 1;
			typedef std::uint32_t field_index_data_ver_type;
			#define field_index_data_ver_type_specifier PRIu32
			static constexpr unsigned const s_field_index_data_ver_position_idx = 1u << 0;
			static constexpr char const s_field_index_data_conn_name[] = "conn";
			static constexpr int const s_field_index_data_conn_name_len = static_cast<int>(std::size(s_field_index_data_conn_name)) - 1;
			typedef std::uint32_t field_index_data_conn_type;
			#define field_index_data_conn_type_specifier PRIu32
			static constexpr unsigned const s_field_index_data_conn_position_idx = 1u << 1;
			static constexpr char const s_field_index_data_count_name[] = "count";
			static constexpr int const s_field_index_data_count_name_len = static_cast<int>(std::size(s_field_index_data_count_name)) - 1;
			typedef std::uint32_t field_index_data_count_type;
			#define field_index_data_count_type_specifier PRIu32
			static constexpr unsigned const s_field_index_data_count_position_idx = 1u << 2;

			static constexpr char const s_field_chunk_info_ver_name[] = "ver";
			static constexpr int const s_field_chunk_info_ver_name_len = static_cast<int>(std::size(s_field_chunk_info_ver_name)) - 1;
			typedef std::uint32_t field_chunk_info_ver_type;
			#define field_chunk_info_ver_type_specifier PRIu32
			static constexpr unsigned const s_field_chunk_info_ver_position_idx = 1u << 0;
			static constexpr char const s_field_chunk_info_chunk_pos_name[] = "chunk_pos";
			static constexpr int const s_field_chunk_info_chunk_pos_name_len = static_cast<int>(std::size(s_field_chunk_info_chunk_pos_name)) - 1;
			typedef std::uint64_t field_chunk_info_chunk_pos_type;
			#define field_chunk_info_chunk_pos_type_specifier PRIu64
			static constexpr unsigned const s_field_chunk_info_chunk_pos_position_idx = 1u << 1;
			static constexpr char const s_field_chunk_info_start_time_name[] = "start_time";
			static constexpr int const s_field_chunk_info_start_time_name_len = static_cast<int>(std::size(s_field_chunk_info_start_time_name)) - 1;
			typedef std::uint64_t field_chunk_info_start_time_type;
			#define field_chunk_info_start_time_type_specifier PRIu64
			static constexpr unsigned const s_field_chunk_info_start_time_position_idx = 1u << 2;
			static constexpr char const s_field_chunk_info_end_time_name[] = "end_time";
			static constexpr int const s_field_chunk_info_end_time_name_len = static_cast<int>(std::size(s_field_chunk_info_end_time_name)) - 1;
			typedef std::uint64_t field_chunk_info_end_time_type;
			#define field_chunk_info_end_time_type_specifier PRIu64
			static constexpr unsigned const s_field_chunk_info_end_time_position_idx = 1u << 3;
			static constexpr char const s_field_chunk_info_count_name[] = "count";
			static constexpr int const s_field_chunk_info_count_name_len = static_cast<int>(std::size(s_field_chunk_info_count_name)) - 1;
			typedef std::uint32_t field_chunk_info_count_type;
			#define field_chunk_info_count_type_specifier PRIu32
			static constexpr unsigned const s_field_chunk_info_count_position_idx = 1u << 4;

			enum class op_code : field_op_type
			{
				bag = 0x03,
				chunk = 0x05,
				connection = 0x07,
				message_data = 0x02,
				index_data = 0x04,
				chunk_info = 0x06,
			};
			
			struct header_bag_t
			{
				field_bag_index_pos_type m_index_pos;
				field_bag_conn_count_type m_conn_count;
				field_bag_chunk_count_type m_chunk_count;
			};
			static constexpr unsigned const s_header_bag_positions = (1u << 3) - 1;
			struct header_chunk_t
			{
				field_chunk_compression_type m_compression;
				field_chunk_size_type m_size;
			};
			static constexpr unsigned const s_header_chunk_positions = (1u << 2) - 1;
			struct header_connection_t
			{
				field_connection_conn_type m_conn;
				field_connection_topic_type m_topic;
			};
			static constexpr unsigned const s_header_connection_positions = (1u << 2) - 1;
			struct header_message_data_t
			{
				field_message_data_conn_type m_conn;
				field_message_data_time_type m_time;
			};
			static constexpr unsigned const s_header_message_data_positions = (1u << 2) - 1;
			struct header_index_data_t
			{
				field_index_data_ver_type m_ver;
				field_index_data_conn_type m_conn;
				field_index_data_count_type m_count;
			};
			static constexpr unsigned const s_header_index_data_positions = (1u << 3) - 1;
			struct header_chunk_info_t
			{
				field_chunk_info_ver_type m_ver;
				field_chunk_info_chunk_pos_type m_chunk_pos;
				field_chunk_info_start_time_type m_start_time;
				field_chunk_info_end_time_type m_end_time;
				field_chunk_info_count_type m_count;
			};
			static constexpr unsigned const s_header_chunk_info_positions = (1u << 5) - 1;

			union header_t
			{
				header_bag_t m_bag;
				header_chunk_t m_chunk;
				header_connection_t m_connection;
				header_message_data_t m_message_data;
				header_index_data_t m_index_data;
				header_chunk_info_t m_chunk_info;
			};

			bool print_info(nchar const* const file_path);
			bool print_info(span_t& file_span);

			bool is_ascii(char const ch);
			bool is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, char const* const target_name_end);
			bool is_field_op_name(char const* const field_name_begin, char const* const field_name_end);

			bool is_valid_op(field_op_type const op);
			char const* op_to_string(field_op_type const op);
			header_t op_to_header(field_op_type const op);

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
		}
	}
}


bool mk::rosbag::print_info(int const argc, nchar const* const* const argv)
{
	CHECK_RET_F(argc >= 2);
	for(int i = 1; i != argc; ++i)
	{
		bool const printed = detail::print_info(argv[i]);
		CHECK_RET_F(printed);
	}
	return true;
}


bool mk::rosbag::detail::print_info(nchar const* const file_path)
{
	mk::read_only_memory_mapped_file_t const rommf{file_path};
	CHECK_RET_F(rommf);

	span_t file_span{rommf.get_data(), rommf.get_size()};
	bool const printed = detail::print_info(file_span);
	CHECK_RET_F(printed);

	return true;
}

bool mk::rosbag::detail::print_info(span_t& file_span)
{
	CHECK_RET_F(has_magic(file_span));
	consume_magic(file_span);

	int record_idx = 0;
	while(file_span.m_len != 0)
	{
		CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const header_len = read<std::uint32_t>(file_span);
		CHECK_RET_F(header_len <= file_span.m_len);
		span_t header_span = span_t{file_span.m_ptr, header_len};
		detail::field_op_type op{};
		bool op_found = false;
		while(header_span.m_len != 0)
		{
			CHECK_RET_F(header_span.m_len >= sizeof(std::uint32_t));
			std::uint32_t const field_len = read<std::uint32_t>(header_span);
			CHECK_RET_F(field_len <= header_span.m_len);
			char const* const field_begin = static_cast<char const*>(header_span.m_ptr);
			char const* const field_end = field_begin + field_len;
			auto const field_name_end = std::find(field_begin, field_end, '=');
			CHECK_RET_F(field_name_end != field_end);
			CHECK_RET_F(field_name_end != field_begin);
			CHECK_RET_F(std::all_of(field_begin, field_name_end, detail::is_ascii));
			std::uint32_t const field_name_len = static_cast<std::uint32_t>(field_name_end - field_begin);
			consume(header_span, field_name_len + 1);
			std::uint32_t const field_data_len = field_len - field_name_len - 1;
			if(detail::is_field_op_name(field_begin, field_name_end))
			{
				CHECK_RET_F(field_data_len == sizeof(detail::field_op_type));
				op = read<detail::field_op_type>(header_span);
				op_found = true;
				break;
			}
			consume(header_span, field_data_len);
		}
		CHECK_RET_F(op_found);
		mk_printf("Record #%d, type = %s", record_idx, detail::op_to_string(op));

		detail::header_t header = detail::op_to_header(op);
		unsigned header_filled = 0;

		header_span = span_t{file_span.m_ptr, header_len};
		while(header_span.m_len != 0)
		{
			CHECK_RET_F(header_span.m_len >= sizeof(std::uint32_t));
			std::uint32_t const field_len = read<std::uint32_t>(header_span);
			CHECK_RET_F(field_len <= header_span.m_len);
			char const* const field_begin = static_cast<char const*>(header_span.m_ptr);
			char const* const field_end = field_begin + field_len;
			auto const field_name_end = std::find(field_begin, field_end, '=');
			CHECK_RET_F(field_name_end != field_end);
			CHECK_RET_F(field_name_end != field_begin);
			CHECK_RET_F(std::all_of(field_begin, field_name_end, detail::is_ascii));
			std::uint32_t const field_name_len = static_cast<std::uint32_t>(field_name_end - field_begin);
			consume(header_span, field_name_len + 1);
			std::uint32_t const field_data_len = field_len - field_name_len - 1;
			if(detail::is_field_op_name(field_begin, field_name_end))
			{
				consume(header_span, field_data_len);
				continue;
			}
			switch(op)
			{
				case static_cast<std::uint8_t>(detail::op_code::bag):
				{
					if(detail::is_field_bag_index_pos_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_index_pos_type));
						detail::field_bag_index_pos_type const val = read<detail::field_bag_index_pos_type>(header_span);
						header_filled |= detail::s_field_bag_index_pos_position_idx;
						header.m_bag.m_index_pos = val;
						mk_printf(", %s = %" field_bag_index_pos_type_specifier "", detail::s_field_bag_index_pos_name, val);
					}
					else if(detail::is_field_bag_conn_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_conn_count_type));
						detail::field_bag_conn_count_type const val = read<detail::field_bag_conn_count_type>(header_span);
						header_filled |= detail::s_field_bag_conn_count_position_idx;
						header.m_bag.m_conn_count = val;
						mk_printf(", %s = %" field_bag_conn_count_type_specifier "", detail::s_field_bag_conn_count_name, val);
					}
					else if(detail::is_field_bag_chunk_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_bag_chunk_count_type));
						detail::field_bag_chunk_count_type const val = read<detail::field_bag_chunk_count_type>(header_span);
						header_filled |= detail::s_field_bag_chunk_count_position_idx;
						header.m_bag.m_chunk_count = val;
						mk_printf(", %s = %" field_bag_chunk_count_type_specifier "", detail::s_field_bag_chunk_count_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::chunk):
				{
					if(detail::is_field_chunk_compression_name(field_begin, field_name_end))
					{
						detail::field_chunk_compression_type const val = {field_name_end + 1, static_cast<int>(field_data_len)};
						CHECK_RET_F(std::all_of(val.m_begin, val.m_begin + val.m_len, detail::is_ascii));
						header_filled |= detail::s_field_chunk_compression_position_idx;
						header.m_chunk.m_compression = val;
						mk_printf(", %s = %" field_chunk_compression_type_specifier "", detail::s_field_chunk_compression_name, val.m_len, val.m_begin);
						consume(header_span, field_data_len);
					}
					else if(detail::is_field_chunk_size_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_size_type));
						detail::field_chunk_size_type const val = read<detail::field_chunk_size_type>(header_span);
						header_filled |= detail::s_field_chunk_size_position_idx;
						header.m_chunk.m_size = val;
						mk_printf(", %s = %" field_chunk_size_type_specifier "", detail::s_field_chunk_size_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::connection):
				{
					if(detail::is_field_connection_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_connection_conn_type));
						detail::field_connection_conn_type const val = read<detail::field_connection_conn_type>(header_span);
						header_filled |= detail::s_field_connection_conn_position_idx;
						header.m_connection.m_conn = val;
						mk_printf(", %s = %" field_connection_conn_type_specifier "", detail::s_field_connection_conn_name, val);
					}
					else if(detail::is_field_connection_topic_name(field_begin, field_name_end))
					{
						detail::field_connection_topic_type const val = {field_name_end + 1, static_cast<int>(field_data_len)};
						CHECK_RET_F(std::all_of(val.m_begin, val.m_begin + val.m_len, detail::is_ascii));
						header_filled |= detail::s_field_connection_topic_position_idx;
						header.m_connection.m_topic = val;
						mk_printf(", %s = %" field_connection_topic_type_specifier "", detail::s_field_connection_topic_name, val.m_len, val.m_begin);
						consume(header_span, field_data_len);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::message_data):
				{
					if(detail::is_field_message_data_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_message_data_conn_type));
						detail::field_message_data_conn_type const val = read<detail::field_message_data_conn_type>(header_span);
						header_filled |= detail::s_field_message_data_conn_position_idx;
						header.m_message_data.m_conn = val;
						mk_printf(", %s = %" field_message_data_conn_type_specifier "", detail::s_field_message_data_conn_name, val);
					}
					else if(detail::is_field_message_data_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_message_data_time_type));
						detail::field_message_data_time_type const val = read<detail::field_message_data_time_type>(header_span);
						header_filled |= detail::s_field_message_data_time_position_idx;
						header.m_message_data.m_time = val;
						mk_printf(", %s = %" field_message_data_time_type_specifier "", detail::s_field_message_data_time_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::index_data):
				{
					if(detail::is_field_index_data_ver_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_ver_type));
						detail::field_index_data_ver_type const val = read<detail::field_index_data_ver_type>(header_span);
						header_filled |= detail::s_field_index_data_ver_position_idx;
						header.m_index_data.m_ver = val;
						mk_printf(", %s = %" field_index_data_ver_type_specifier "", detail::s_field_index_data_ver_name, val);
					}
					else if(detail::is_field_index_data_conn_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_conn_type));
						detail::field_index_data_conn_type const val = read<detail::field_index_data_conn_type>(header_span);
						header_filled |= detail::s_field_index_data_conn_position_idx;
						header.m_index_data.m_conn = val;
						mk_printf(", %s = %" field_index_data_conn_type_specifier "", detail::s_field_index_data_conn_name, val);
					}
					else if(detail::is_field_index_data_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_index_data_count_type));
						detail::field_index_data_count_type const val = read<detail::field_index_data_count_type>(header_span);
						header_filled |= detail::s_field_index_data_count_position_idx;
						header.m_index_data.m_count = val;
						mk_printf(", %s = %" field_index_data_count_type_specifier "", detail::s_field_index_data_count_name, val);
					}
				}
				break;
				case static_cast<std::uint8_t>(detail::op_code::chunk_info):
				{
					if(detail::is_field_chunk_info_ver_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_ver_type));
						detail::field_chunk_info_ver_type const val = read<detail::field_chunk_info_ver_type>(header_span);
						header_filled |= detail::s_field_chunk_info_ver_position_idx;
						header.m_chunk_info.m_ver = val;
						mk_printf(", %s = %" field_chunk_info_ver_type_specifier "", detail::s_field_chunk_info_ver_name, val);
					}
					else if(detail::is_field_chunk_info_chunk_pos_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_chunk_pos_type));
						detail::field_chunk_info_chunk_pos_type const val = read<detail::field_chunk_info_chunk_pos_type>(header_span);
						header_filled |= detail::s_field_chunk_info_chunk_pos_position_idx;
						header.m_chunk_info.m_chunk_pos = val;
						mk_printf(", %s = %" field_chunk_info_chunk_pos_type_specifier "", detail::s_field_chunk_info_chunk_pos_name, val);
					}
					else if(detail::is_field_chunk_info_start_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_start_time_type));
						detail::field_chunk_info_start_time_type const val = read<detail::field_chunk_info_start_time_type>(header_span);
						header_filled |= detail::s_field_chunk_info_start_time_position_idx;
						header.m_chunk_info.m_start_time = val;
						mk_printf(", %s = %" field_chunk_info_start_time_type_specifier "", detail::s_field_chunk_info_start_time_name, val);
					}
					else if(detail::is_field_chunk_info_end_time_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_end_time_type));
						detail::field_chunk_info_end_time_type const val = read<detail::field_chunk_info_end_time_type>(header_span);
						header_filled |= detail::s_field_chunk_info_end_time_position_idx;
						header.m_chunk_info.m_end_time = val;
						mk_printf(", %s = %" field_chunk_info_end_time_type_specifier "", detail::s_field_chunk_info_end_time_name, val);
					}
					else if(detail::is_field_chunk_info_count_name(field_begin, field_name_end))
					{
						CHECK_RET_F(field_data_len == sizeof(detail::field_chunk_info_count_type));
						detail::field_chunk_info_count_type const val = read<detail::field_chunk_info_count_type>(header_span);
						header_filled |= detail::s_field_chunk_info_count_position_idx;
						header.m_chunk_info.m_count = val;
						mk_printf(", %s = %" field_chunk_info_count_type_specifier "", detail::s_field_chunk_info_count_name, val);
					}
				}
				break;
				default:
				{
					mk_printf(", %.*s len %" PRIu32 "", static_cast<int>(field_name_len), field_begin, field_data_len);
					consume(header_span, field_data_len);
				}
				break;
			}
		}
		switch(op)
		{
			case static_cast<std::uint8_t>(detail::op_code::bag):
			{
				CHECK_RET_F(header_filled == detail::s_header_bag_positions);
			}
			break;
			case static_cast<std::uint8_t>(detail::op_code::chunk):
			{
				CHECK_RET_F(header_filled == detail::s_header_chunk_positions);
			}
			break;
			case static_cast<std::uint8_t>(detail::op_code::connection):
			{
				CHECK_RET_F(header_filled == detail::s_header_connection_positions);
			}
			break;
			case static_cast<std::uint8_t>(detail::op_code::message_data):
			{
				CHECK_RET_F(header_filled == detail::s_header_message_data_positions);
			}
			break;
			case static_cast<std::uint8_t>(detail::op_code::index_data):
			{
				CHECK_RET_F(header_filled == detail::s_header_index_data_positions);
			}
			break;
			case static_cast<std::uint8_t>(detail::op_code::chunk_info):
			{
				CHECK_RET_F(header_filled == detail::s_header_chunk_info_positions);
			}
			break;
			default:
			{
			}
			break;
		}
		consume(file_span, header_len);

		CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const record_data_len = read<std::uint32_t>(file_span);
		CHECK_RET_F(record_data_len <= file_span.m_len);
		mk_printf(", record data len = %" PRIu32 "", record_data_len);

		consume(file_span, record_data_len);
		mk_printf("%s", "\n");
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

mk::rosbag::detail::header_t mk::rosbag::detail::op_to_header(field_op_type const op)
{
	header_t header;
	switch(op)
	{
		case static_cast<std::uint8_t>(op_code::bag):
		{
			header.m_bag = header_bag_t{};
		}
		break;
		case static_cast<std::uint8_t>(op_code::chunk):
		{
			header.m_chunk = header_chunk_t{};
		}
		break;
		case static_cast<std::uint8_t>(op_code::connection):
		{
			header.m_connection = header_connection_t{};
		}
		break;
		case static_cast<std::uint8_t>(op_code::message_data):
		{
			header.m_message_data = header_message_data_t{};
		}
		break;
		case static_cast<std::uint8_t>(op_code::index_data):
		{
			header.m_index_data = header_index_data_t{};
		}
		break;
		case static_cast<std::uint8_t>(op_code::chunk_info):
		{
			header.m_chunk_info = header_chunk_info_t{};
		}
		break;
		default:
		{
			std::memset(&header, 0, sizeof(header));
		}
		break;
	}
	return header;
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
