#include "read_only_memory_mapped_file_linux.h"

#include "check_ret.h"

#include <cassert>
#include <cstdint> // std::uint64_t
#include <limits>
#include <utility> // std::swap

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// fstat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/mman.h> // map, munmap
#include <unistd.h> // close


static constexpr int const s_invalid_fd = -1;
static constexpr void* const s_invalid_mapping = nullptr;


mk::read_only_memory_mapped_file_linux_t::read_only_memory_mapped_file_linux_t() noexcept :
	m_fd(s_invalid_fd),
	m_mapping(s_invalid_mapping),
	m_size()
{
}

mk::read_only_memory_mapped_file_linux_t::read_only_memory_mapped_file_linux_t(char const* const& file_path) :
	read_only_memory_mapped_file_linux_t()
{
	int const fd = open(file_path, O_RDONLY, O_CLOEXEC);
	CHECK_RET_V(fd != s_invalid_fd);
	assert(fd >= 0);
	m_fd = fd;

	struct stat stat_buff;
	int const stated = fstat(m_fd, &stat_buff);
	CHECK_RET_V(stated == 0);
	CHECK_RET_V(static_cast<std::uint64_t>(stat_buff.st_size) <= static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)()));
	m_size = static_cast<std::size_t>(stat_buff.st_size);

	void* const mapping = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
	CHECK_RET_V(mapping != MAP_FAILED && mapping != s_invalid_mapping);
	m_mapping = mapping;
}

mk::read_only_memory_mapped_file_linux_t::read_only_memory_mapped_file_linux_t(read_only_memory_mapped_file_linux_t&& other) noexcept :
	read_only_memory_mapped_file_linux_t()
{
	swap(other);
}

mk::read_only_memory_mapped_file_linux_t& mk::read_only_memory_mapped_file_linux_t::operator=(read_only_memory_mapped_file_linux_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_memory_mapped_file_linux_t::~read_only_memory_mapped_file_linux_t() noexcept
{
	if(m_mapping != s_invalid_mapping)
	{
		int const munmapped = munmap(m_mapping, m_size);
		CHECK_RET_V(munmapped == 0);
	}
	if(m_fd != s_invalid_fd)
	{
		int const closed = close(m_fd);
		CHECK_RET_V(closed == 0);
	}
}

void mk::read_only_memory_mapped_file_linux_t::swap(read_only_memory_mapped_file_linux_t& other) noexcept
{
	using std::swap;
	swap(m_fd, other.m_fd);
	swap(m_mapping, other.m_mapping);
	swap(m_size, other.m_size);
}

mk::read_only_memory_mapped_file_linux_t::operator bool() const
{
	return m_mapping != s_invalid_mapping;
}

void const* mk::read_only_memory_mapped_file_linux_t::get_data() const
{
	return m_mapping;
}

std::size_t mk::read_only_memory_mapped_file_linux_t::get_size() const
{
	return m_size;
}
