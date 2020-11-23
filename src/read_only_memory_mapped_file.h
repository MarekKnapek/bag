#pragma once


#include "native.h"

#include <cstddef> // std::size_t


#ifdef _MSC_VER
	#include "read_only_memory_mapped_file_windows.h"
	typedef mk::read_only_memory_mapped_file_windows_t read_only_memory_mapped_file_native_t;
#else
	#include "read_only_memory_mapped_file_linux.h"
	typedef mk::read_only_memory_mapped_file_linux_t read_only_memory_mapped_file_native_t;
#endif


namespace mk
{


	class read_only_memory_mapped_file_t
	{
	public:
		read_only_memory_mapped_file_t() noexcept;
		read_only_memory_mapped_file_t(nchar const* const file_path);
		read_only_memory_mapped_file_t(read_only_memory_mapped_file_t const&) = delete;
		read_only_memory_mapped_file_t(read_only_memory_mapped_file_t&& other) noexcept;
		read_only_memory_mapped_file_t& operator=(read_only_memory_mapped_file_t const&) = delete;
		read_only_memory_mapped_file_t& operator=(read_only_memory_mapped_file_t&& other) noexcept;
		~read_only_memory_mapped_file_t() noexcept;
		void swap(read_only_memory_mapped_file_t& other) noexcept;
	public:
		explicit operator bool() const;
		void const* get_data() const;
		std::size_t get_size() const;
	private:
		read_only_memory_mapped_file_native_t m_native_file;
	};

	inline void swap(read_only_memory_mapped_file_t& a, read_only_memory_mapped_file_t& b) noexcept { a.swap(b); }


}
