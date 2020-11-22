#pragma once


#include <cstddef> // std::size_t


namespace mk
{


	class read_only_memory_mapped_file_windows_t
	{
	public:
		read_only_memory_mapped_file_windows_t() noexcept;
		read_only_memory_mapped_file_windows_t(wchar_t const* const file_path);
		read_only_memory_mapped_file_windows_t(read_only_memory_mapped_file_windows_t const&) = delete;
		read_only_memory_mapped_file_windows_t(read_only_memory_mapped_file_windows_t&& other) noexcept;
		read_only_memory_mapped_file_windows_t& operator=(read_only_memory_mapped_file_windows_t const&) = delete;
		read_only_memory_mapped_file_windows_t& operator=(read_only_memory_mapped_file_windows_t&& other) noexcept;
		~read_only_memory_mapped_file_windows_t() noexcept;
		void swap(read_only_memory_mapped_file_windows_t& other) noexcept;
	public:
		explicit operator bool() const;
		void const* get_data() const;
		std::size_t get_size() const;
	private:
		void* m_file;
		void* m_mapping;
		void const* m_view;
		std::size_t m_size;
	};

	inline void swap(read_only_memory_mapped_file_windows_t& a, read_only_memory_mapped_file_windows_t& b) noexcept { a.swap(b); }

	typedef read_only_memory_mapped_file_windows_t read_only_memory_mapped_file_native_t;


}
