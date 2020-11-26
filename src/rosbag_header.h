#pragma once


#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t


namespace mk { namespace rosbag { struct span_t; } }


namespace mk
{
	namespace rosbag
	{


		typedef std::uint8_t field_op_type;
		enum class header_type : field_op_type
		{
			bag = 0x03,
			chunk = 0x05,
			connection = 0x07,
			message_data = 0x02,
			index_data = 0x04,
			chunk_info = 0x06,
		};

		struct string_t
		{
			char const* m_begin;
			int m_len;
		};


		typedef std::uint64_t field_bag_index_pos_type;
		typedef std::uint32_t field_bag_conn_count_type;
		typedef std::uint32_t field_bag_chunk_count_type;
		struct header_bag_t
		{
			field_bag_index_pos_type m_index_pos;
			field_bag_conn_count_type m_conn_count;
			field_bag_chunk_count_type m_chunk_count;
		};

		typedef string_t field_chunk_compression_type;
		typedef std::uint32_t field_chunk_size_type;
		struct header_chunk_t
		{
			field_chunk_compression_type m_compression;
			field_chunk_size_type m_size;
		};

		typedef std::uint32_t field_connection_conn_type;
		typedef string_t field_connection_topic_type;
		struct header_connection_t
		{
			field_connection_conn_type m_conn;
			field_connection_topic_type m_topic;
		};

		typedef std::uint32_t field_message_data_conn_type;
		typedef std::uint64_t field_message_data_time_type;
		struct header_message_data_t
		{
			field_message_data_conn_type m_conn;
			field_message_data_time_type m_time;
		};

		typedef std::uint32_t field_index_data_ver_type;
		typedef std::uint32_t field_index_data_conn_type;
		typedef std::uint32_t field_index_data_count_type;
		struct header_index_data_t
		{
			field_index_data_ver_type m_ver;
			field_index_data_conn_type m_conn;
			field_index_data_count_type m_count;
		};

		typedef std::uint32_t field_chunk_info_ver_type;
		typedef std::uint64_t field_chunk_info_chunk_pos_type;
		typedef std::uint64_t field_chunk_info_start_time_type;
		typedef std::uint64_t field_chunk_info_end_time_type;
		typedef std::uint32_t field_chunk_info_count_type;
		struct header_chunk_info_t
		{
			field_chunk_info_ver_type m_ver;
			field_chunk_info_chunk_pos_type m_chunk_pos;
			field_chunk_info_start_time_type m_start_time;
			field_chunk_info_end_time_type m_end_time;
			field_chunk_info_count_type m_count;
		};

		union header_t
		{
			header_bag_t m_bag;
			header_chunk_t m_chunk;
			header_connection_t m_connection;
			header_message_data_t m_message_data;
			header_index_data_t m_index_data;
			header_chunk_info_t m_chunk_info;
		};

		typedef string_t field_connection_data_topic_t;
		typedef string_t field_connection_data_type_t;
		typedef string_t field_connection_data_md5sum_t;
		typedef string_t field_connection_data_message_definition_t;
		struct header_connection_data_t
		{
			field_connection_data_topic_t m_topic;
			field_connection_data_type_t m_type;
			field_connection_data_md5sum_t m_md5sum;
			field_connection_data_message_definition_t m_message_definition;
		};

		bool read_header_type(span_t file_span, bool* const out_op_found, header_type* const out_header_type);
		bool read_header(span_t& file_span, header_type const header_type, header_t* const out_header);
		bool read_header(span_t& file_span, header_bag_t* const out_header);
		bool read_header(span_t& file_span, header_chunk_t* const out_header);
		bool read_header(span_t& file_span, header_connection_t* const out_header);
		bool read_header(span_t& file_span, header_message_data_t* const out_header);
		bool read_header(span_t& file_span, header_index_data_t* const out_header);
		bool read_header(span_t& file_span, header_chunk_info_t* const out_header);
		bool read_header(span_t& file_span, header_connection_data_t* const out_header);


	}
}
