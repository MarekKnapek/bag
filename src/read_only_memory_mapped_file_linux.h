#pragma once


#include <cstddef> // std::size_t


namespace mk
{


	class read_only_memory_mapped_file_linux_t
	{
	public:
		read_only_memory_mapped_file_linux_t() noexcept;
		read_only_memory_mapped_file_linux_t(char const* const& file_path);
		read_only_memory_mapped_file_linux_t(read_only_memory_mapped_file_linux_t const&) = delete;
		read_only_memory_mapped_file_linux_t(read_only_memory_mapped_file_linux_t&& other) noexcept;
		read_only_memory_mapped_file_linux_t& operator=(read_only_memory_mapped_file_linux_t const&) = delete;
		read_only_memory_mapped_file_linux_t& operator=(read_only_memory_mapped_file_linux_t&& other) noexcept;
		~read_only_memory_mapped_file_linux_t() noexcept;
		void swap(read_only_memory_mapped_file_linux_t& other) noexcept;
	public:
		explicit operator bool() const;
		void const* get_data() const;
		std::size_t get_size() const;
	private:
		int m_fd;
		void* m_mapping;
		std::size_t m_size;
	};

	inline void swap(read_only_memory_mapped_file_linux_t& a, read_only_memory_mapped_file_linux_t& b) noexcept { a.swap(b); }


}
