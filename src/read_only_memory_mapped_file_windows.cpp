#include "read_only_memory_mapped_file_windows.h"

#include "check_ret.h"

#include <cstdint> // std::uint64_t
#include <limits>
#include <utility> // std::swap

#include <windows.h>


static constexpr HANDLE const s_invalid_file = nullptr;
static constexpr void* const s_invalid_mapping = nullptr;
static constexpr void* const s_invalid_view = nullptr;


mk::read_only_memory_mapped_file_windows_t::read_only_memory_mapped_file_windows_t() noexcept :
	m_file(s_invalid_file),
	m_mapping(s_invalid_mapping),
	m_view(s_invalid_view),
	m_size()
{
}

mk::read_only_memory_mapped_file_windows_t::read_only_memory_mapped_file_windows_t(wchar_t const* const file_path) :
	read_only_memory_mapped_file_windows_t()
{
	HANDLE const file = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	CHECK_RET_V(file != INVALID_HANDLE_VALUE && file != s_invalid_file);
	m_file = file;

	LARGE_INTEGER size;
	BOOL const got_size = GetFileSizeEx(file, &size);
	CHECK_RET_V(got_size != 0);
	CHECK_RET_V(static_cast<std::uint64_t>(size.QuadPart) <= static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)()));
	m_size = static_cast<std::size_t>(size.QuadPart);

	HANDLE const mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CHECK_RET_V(mapping != s_invalid_mapping);
	m_mapping = mapping;

	void const* const view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	CHECK_RET_V(view != s_invalid_view);
	m_view = view;
}

mk::read_only_memory_mapped_file_windows_t::read_only_memory_mapped_file_windows_t(read_only_memory_mapped_file_windows_t&& other) noexcept :
	read_only_memory_mapped_file_windows_t()
{
	swap(other);
}

mk::read_only_memory_mapped_file_windows_t& mk::read_only_memory_mapped_file_windows_t::operator=(read_only_memory_mapped_file_windows_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_memory_mapped_file_windows_t::~read_only_memory_mapped_file_windows_t() noexcept
{
	if(m_view != s_invalid_view)
	{
		BOOL const unmapped = UnmapViewOfFile(m_view);
		CHECK_RET_V(unmapped != 0);
	}
	if(m_mapping != s_invalid_mapping)
	{
		BOOL const closed = CloseHandle(m_mapping);
		CHECK_RET_V(closed != 0);
	}
	if(m_file != s_invalid_file)
	{
		BOOL const closed = CloseHandle(m_file);
		CHECK_RET_V(closed != 0);
	}
}

void mk::read_only_memory_mapped_file_windows_t::swap(read_only_memory_mapped_file_windows_t& other) noexcept
{
	using std::swap;
	swap(m_file, other.m_file);
	swap(m_mapping, other.m_mapping);
	swap(m_view, other.m_view);
	swap(m_size, other.m_size);
}

mk::read_only_memory_mapped_file_windows_t::operator bool() const
{
	return m_view != s_invalid_view;
}

void const* mk::read_only_memory_mapped_file_windows_t::get_data() const
{
	return m_view;
}

std::size_t mk::read_only_memory_mapped_file_windows_t::get_size() const
{
	return m_size;
}
