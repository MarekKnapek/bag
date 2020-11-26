#include "rosbag_pcap.h"

#include "scope_exit.h"
#include "check_ret.h"
#include "printf.h"
#include "read_only_memory_mapped_file.h"
#include "rosbag.h"
#include "rosbag_detail.h"
#include "rosbag_header.h"

#include <algorithm> // std::find
#include <cassert>
#include <cinttypes> // PRIu32, PRIu64
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t
#include <cstdio> // std::printf, std::puts
#include <cstring> // std::memset
#include <iterator> // std::size
#include <limits> // std::numeric_limits<std::size_t>::max;
#include <vector>

#include <lz4frame.h>


namespace mk
{
	namespace rosbag_pcap
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

			static constexpr char const s_field_connection_data_topic_name[] = "topic";
			static constexpr int const s_field_connection_data_topic_name_len = static_cast<int>(std::size(s_field_connection_data_topic_name)) - 1;
			typedef string_t field_connection_data_topic_type;
			#define field_connection_data_topic_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_topic_position_idx = 1u << 0;
			static constexpr char const s_field_connection_data_type_name[] = "type";
			static constexpr int const s_field_connection_data_type_name_len = static_cast<int>(std::size(s_field_connection_data_type_name)) - 1;
			typedef string_t field_connection_data_type_type;
			#define field_connection_data_type_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_type_position_idx = 1u << 1;
			static constexpr char const s_field_connection_data_md5sum_name[] = "md5sum";
			static constexpr int const s_field_connection_data_md5sum_name_len = static_cast<int>(std::size(s_field_connection_data_md5sum_name)) - 1;
			typedef string_t field_connection_data_md5sum_type;
			#define field_connection_data_md5sum_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_md5sum_position_idx = 1u << 2;
			static constexpr char const s_field_connection_data_message_definition_name[] = "message_definition";
			static constexpr int const s_field_connection_data_message_definition_name_len = static_cast<int>(std::size(s_field_connection_data_message_definition_name)) - 1;
			typedef string_t field_connection_data_message_definition_type;
			#define field_connection_data_message_definition_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_message_definition_position_idx = 1u << 3;
			static constexpr char const s_field_connection_data_callerid_name[] = "callerid";
			static constexpr int const s_field_connection_data_callerid_name_len = static_cast<int>(std::size(s_field_connection_data_callerid_name)) - 1;
			typedef string_t field_connection_data_callerid_type;
			#define field_connection_data_callerid_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_callerid_position_idx = 1u << 4;
			static constexpr char const s_field_connection_data_latching_name[] = "latching";
			static constexpr int const s_field_connection_data_latching_name_len = static_cast<int>(std::size(s_field_connection_data_latching_name)) - 1;
			typedef string_t field_connection_data_latching_type;
			#define field_connection_data_latching_type_specifier ".*s"
			static constexpr unsigned const s_field_connection_data_latching_position_idx = 1u << 5;

			struct header_connection_data_t
			{
				field_connection_data_topic_type m_topic;
				field_connection_data_type_type m_type;
				field_connection_data_md5sum_type m_md5sum;
				field_connection_data_message_definition_type m_message_definition;
				field_connection_data_callerid_type m_callerid;
				field_connection_data_latching_type m_latching;
			};
			static constexpr unsigned const s_header_connection_data_positions = s_field_connection_data_topic_position_idx | s_field_connection_data_type_position_idx | s_field_connection_data_md5sum_position_idx | s_field_connection_data_message_definition_position_idx;

			static constexpr char const s_ouster_lidar_topic_name[] = "/os_node/lidar_packets";
			static constexpr int const s_ouster_lidar_topic_name_len = static_cast<int>(std::size(s_ouster_lidar_topic_name)) - 1;

			bool pcap(nchar const* const file_path);
			bool pcap(mk::rosbag::span_t const& file_span);
			bool find_ouster_lidar_connection(mk::rosbag::span_t const& file_span, bool* const out_found, std::uint32_t* const out_connection);

			bool is_ascii(char const ch);
			bool is_ascii_with_newlines_and_tabs(char const ch);
			bool is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, char const* const target_name_end);
			bool is_field_op_name(char const* const field_name_begin, char const* const field_name_end);

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


bool mk::rosbag_pcap::pcap(int const argc, nchar const* const* const argv)
{
	CHECK_RET_F(argc >= 2);
	for(int i = 1; i != argc; ++i)
	{
		bool const bussiness = detail::pcap(argv[i]);
		CHECK_RET_F(bussiness);
	}
	return true;
}


bool mk::rosbag_pcap::detail::pcap(nchar const* const file_path)
{
	mk::read_only_memory_mapped_file_t const rommf{file_path};
	CHECK_RET_F(rommf);

	mk::rosbag::span_t file_span{rommf.get_data(), rommf.get_size()};
	bool const bussiness = detail::pcap(file_span);
	CHECK_RET_F(bussiness);

	return true;
}

bool mk::rosbag_pcap::detail::pcap(mk::rosbag::span_t const& orig_file_span)
{
	bool found;
	std::uint32_t connection;
	bool const search_ok = find_ouster_lidar_connection(orig_file_span, &found, &connection);
	CHECK_RET_F(search_ok);
	CHECK_RET_F(found);

	// TODO: Read chunk and all its following index_data.
	// See, if there is my connection, if it is then decompress chunk and jump to connection index.
	// Serialize decompressed data.

	mk::rosbag::span_t file_span = orig_file_span;
	mk::rosbag::consume_magic(file_span);

	while(file_span.m_len != 0)
	{
		bool op_found;
		mk::rosbag::header_type header_type;
		bool const header_type_read = mk::rosbag::read_header_type(file_span, &op_found, &header_type);
		CHECK_RET_F(header_type_read);
		CHECK_RET_F(op_found);
		if(header_type != mk::rosbag::header_type::chunk)
		{
			std::uint32_t const header_len = mk::rosbag::read<std::uint32_t>(file_span);
			mk::rosbag::consume(file_span, header_len);
			CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
			std::uint32_t const data_len = mk::rosbag::read<std::uint32_t>(file_span);
			mk::rosbag::consume(file_span, data_len);
			continue;
		}
		mk::rosbag::header_chunk_t header_chunk;
		bool const header_chunk_read = mk::rosbag::read_header(file_span, &header_chunk);
		CHECK_RET_F(header_chunk_read);
		CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const data_len = mk::rosbag::read<std::uint32_t>(file_span);
		mk::rosbag::span_t chunk_data{file_span.m_ptr, data_len};
		mk::rosbag::consume(file_span, data_len);
		bool header_index_data_found = false;
		mk::rosbag::header_index_data_t header_index_data{};
		for(;;)
		{
			bool op_found_2;
			mk::rosbag::header_type header_type_2;
			bool const header_type_read_2 = mk::rosbag::read_header_type(file_span, &op_found_2, &header_type_2);
			CHECK_RET_F(header_type_read_2);
			CHECK_RET_F(op_found_2);
			if(header_type_2 != mk::rosbag::header_type::index_data)
			{
				break;
			}
			bool const header_index_data_read = mk::rosbag::read_header(file_span, &header_index_data);
			CHECK_RET_F(header_index_data_read);
			if(header_index_data.m_conn == connection && header_index_data.m_ver == 1)
			{
				header_index_data_found = true;
				break;
			}
			CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
			std::uint32_t const data_len_2 = mk::rosbag::read<std::uint32_t>(file_span);
			mk::rosbag::consume(file_span, data_len_2);
		}
		CHECK_RET_F(header_index_data_found);
		CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const index_data_len = mk::rosbag::read<std::uint32_t>(file_span);
		CHECK_RET_F(index_data_len == header_index_data.m_count * (sizeof(std::uint64_t) + sizeof(std::uint32_t)));
		mk::rosbag::span_t index_data_span{file_span.m_ptr, index_data_len};
		mk::rosbag::consume(file_span, index_data_len);

		LZ4F_dctx* ctx;
		auto const ctx_created = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
		CHECK_RET_F(ctx_created == 0);
		auto ctx_free = mk::make_scope_exit([&](){ auto const ctx_freed = LZ4F_freeDecompressionContext(ctx); CHECK_RET_V(ctx_freed == 0); });
		std::vector<unsigned char> decompressed_data;
		decompressed_data.resize(header_chunk.m_size);
		std::size_t dst_size = decompressed_data.size();
		std::size_t src_size = chunk_data.m_len;
		auto const decompressed = LZ4F_decompress(ctx, decompressed_data.data(), &dst_size, chunk_data.m_ptr, &src_size, nullptr);
		CHECK_RET_F(decompressed == 0);
		CHECK_RET_F(dst_size == decompressed_data.size());
		CHECK_RET_F(src_size == chunk_data.m_len);
		ctx_free.reset();
		auto const ctx_freed = LZ4F_freeDecompressionContext(ctx);
		CHECK_RET_F(ctx_freed == 0);

		for(std::uint32_t i = 0; i != header_index_data.m_count; ++i)
		{
			std::uint64_t const time = mk::rosbag::read<std::uint64_t>(index_data_span);
			std::uint32_t const offset = mk::rosbag::read<std::uint32_t>(index_data_span);
			CHECK_RET_F(header_chunk.m_size >= offset);
			CHECK_RET_F(header_chunk.m_size >= offset + 12000);

			mk::rosbag::span_t packet_span{decompressed_data.data() + offset, header_chunk.m_size - offset};
			bool ht_op;
			mk::rosbag::header_type ht;
			bool const ht_read = mk::rosbag::read_header_type(packet_span, &ht_op, &ht);
			CHECK_RET_F(ht_read);
			CHECK_RET_F(ht_op);
			CHECK_RET_F(ht == mk::rosbag::header_type::message_data);
			mk::rosbag::header_message_data_t header_message_data;
			bool const header_message_data_read = mk::rosbag::read_header(packet_span, &header_message_data);
			CHECK_RET_F(header_message_data_read);
			CHECK_RET_F(header_message_data.m_conn == connection);
			CHECK_RET_F(header_message_data.m_time == time);
			CHECK_RET_F(packet_span.m_len >= sizeof(std::uint32_t));
			std::uint32_t const message_data_len = mk::rosbag::read<std::uint32_t>(packet_span);
			CHECK_RET_F(message_data_len == 12613);
			mk::rosbag::span_t message_data_span{packet_span.m_ptr, message_data_len};
			mk::rosbag::consume(message_data_span, 4);
			std::uint64_t const timestamp = mk::rosbag::read<std::uint64_t>(message_data_span);
			(void)timestamp;
		}
	}

	return true;
}

bool mk::rosbag_pcap::detail::find_ouster_lidar_connection(mk::rosbag::span_t const& orig_file_span, bool* const out_found, std::uint32_t* const out_connection)
{
	assert(out_found);
	assert(out_connection);

	mk::rosbag::span_t file_span = orig_file_span;

	bool const has_magic = mk::rosbag::has_magic(file_span);
	CHECK_RET_F(has_magic);
	mk::rosbag::consume_magic(file_span);

	bool op_found;
	mk::rosbag::header_type ht;
	bool const ht_read = mk::rosbag::read_header_type(file_span, &op_found, &ht);
	CHECK_RET_F(ht_read);
	CHECK_RET_F(op_found);
	CHECK_RET_F(ht == mk::rosbag::header_type::bag);

	mk::rosbag::header_bag_t header_bag;
	bool const header_bag_read = mk::rosbag::read_header(file_span, &header_bag);
	CHECK_RET_F(header_bag_read);

	file_span = orig_file_span;
	CHECK_RET_F(header_bag.m_index_pos <= (std::numeric_limits<std::size_t>::max)());
	CHECK_RET_F(file_span.m_len >= static_cast<std::size_t>(header_bag.m_index_pos));
	mk::rosbag::consume(file_span, static_cast<std::size_t>(header_bag.m_index_pos));

	for(std::uint32_t i = 0; i != header_bag.m_conn_count; ++i)
	{
		bool connection_header_op_found;
		mk::rosbag::header_type connection_header_type;
		bool const connection_header_type_read = mk::rosbag::read_header_type(file_span, &connection_header_op_found, &connection_header_type);
		CHECK_RET_F(connection_header_type_read);
		CHECK_RET_F(connection_header_op_found);
		CHECK_RET_F(connection_header_type == mk::rosbag::header_type::connection);

		mk::rosbag::header_connection_t connection_header;
		bool const connection_header_read = mk::rosbag::read_header(file_span, &connection_header);
		CHECK_RET_F(connection_header_read);

		static constexpr char const s_os[] = "/os_node/lidar_packets";
		static constexpr int const s_os_len = static_cast<int>(std::size(s_os)) - 1;
		if(connection_header.m_topic.m_len == s_os_len && std::memcmp(connection_header.m_topic.m_begin, s_os, s_os_len) == 0)
		{
			*out_found = true;
			*out_connection = connection_header.m_conn;
			return true;
		}

		CHECK_RET_F(file_span.m_len >= sizeof(std::uint32_t));
		std::uint32_t const connection_data_len = mk::rosbag::read<std::uint32_t>(file_span);
		CHECK_RET_F(file_span.m_len >= connection_data_len);
		mk::rosbag::consume(file_span, connection_data_len);
	}

	*out_found = false;
	return true;
}


bool mk::rosbag_pcap::detail::is_ascii(char const ch)
{
	unsigned char const uch = static_cast<unsigned char>(ch);
	bool const is_valid = uch >= 0x20 && uch <= 0x7e;
	return is_valid;
}

bool mk::rosbag_pcap::detail::is_ascii_with_newlines_and_tabs(char const ch)
{
	unsigned char const uch = static_cast<unsigned char>(ch);
	bool const is_valid = (uch >= 0x20 && uch <= 0x7e) || (uch == 0x09 || uch == 0x0a || uch == 0x0d);
	return is_valid;
}

bool mk::rosbag_pcap::detail::is_field_name(char const* const field_name_begin, char const* const field_name_end, char const* const target_name_begin, char const* const target_name_end)
{
	bool const is = field_name_end - field_name_begin == target_name_end - target_name_begin && std::memcmp(field_name_begin, target_name_begin, target_name_end - target_name_begin) == 0;
	return is;
}

bool mk::rosbag_pcap::detail::is_field_op_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_op_name, s_field_op_name + s_field_op_name_len);
	return is;
}

char const* mk::rosbag_pcap::detail::op_to_string(field_op_type const op)
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

mk::rosbag_pcap::detail::header_t mk::rosbag_pcap::detail::op_to_header(field_op_type const op)
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


bool mk::rosbag_pcap::detail::is_field_bag_index_pos_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_index_pos_name, s_field_bag_index_pos_name + s_field_bag_index_pos_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_bag_conn_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_conn_count_name, s_field_bag_conn_count_name + s_field_bag_conn_count_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_bag_chunk_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_bag_chunk_count_name, s_field_bag_chunk_count_name + s_field_bag_chunk_count_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_compression_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_compression_name, s_field_chunk_compression_name + s_field_chunk_compression_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_size_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_size_name, s_field_chunk_size_name + s_field_chunk_size_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_connection_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_connection_conn_name, s_field_connection_conn_name + s_field_connection_conn_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_connection_topic_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_connection_topic_name, s_field_connection_topic_name + s_field_connection_topic_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_message_data_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_message_data_conn_name, s_field_message_data_conn_name + s_field_message_data_conn_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_message_data_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_message_data_time_name, s_field_message_data_time_name + s_field_message_data_time_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_index_data_ver_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_ver_name, s_field_index_data_ver_name + s_field_index_data_ver_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_index_data_conn_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_conn_name, s_field_index_data_conn_name + s_field_index_data_conn_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_index_data_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_index_data_count_name, s_field_index_data_count_name + s_field_index_data_count_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_info_ver_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_ver_name, s_field_chunk_info_ver_name + s_field_chunk_info_ver_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_info_chunk_pos_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_chunk_pos_name, s_field_chunk_info_chunk_pos_name + s_field_chunk_info_chunk_pos_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_info_start_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_start_time_name, s_field_chunk_info_start_time_name + s_field_chunk_info_start_time_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_info_end_time_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_end_time_name, s_field_chunk_info_end_time_name + s_field_chunk_info_end_time_name_len);
	return is;
}

bool mk::rosbag_pcap::detail::is_field_chunk_info_count_name(char const* const field_name_begin, char const* const field_name_end)
{
	bool const is = is_field_name(field_name_begin, field_name_end, s_field_chunk_info_count_name, s_field_chunk_info_count_name + s_field_chunk_info_count_name_len);
	return is;
}
