#pragma once

#include "vfs.h"

#include <string>
#include <fstream>
#include <vector>
#include <iostream>

#include <assert.h>

namespace fileio
{
	//-------------------------------------------------------------------------
	template<typename TAlloc = std::allocator<std::byte>>
	std::vector<std::byte, TAlloc> read_binary_file(const std::string& filename)
	{
		// Load file from disk
		const std::string path = vfs::resolve_path(filename);

		std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			std::cerr << "[fileio] File " << filename << " with full path " << path << " was not found!" << std::endl;
			return {};
		}
		else
		{
			std::cerr << "[fileio] Binary file read at path: " << path << std::endl;
		}

		// Read file
		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<std::byte, TAlloc> buffer(size);
		if (file.read((char*)buffer.data(), size))
		{
			return buffer;
		}

		assert(false);
		return {};
	}

	//-------------------------------------------------------------------------
	template<typename TAlloc = std::allocator<char>>
	std::basic_string<char, std::char_traits<char>, TAlloc> read_text_file(const std::string& filename)
	{
		// Load file from disk
		const std::string path = vfs::resolve_path(filename);

		std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			std::cerr << "[fileio] File " << filename << " with full path " << path << " was not found!" << std::endl;
			return {};
		}

		// Read file
		file.seekg(0, std::ios::end);
		const auto size = file.tellg();
		std::basic_string<char, std::char_traits<char>, TAlloc> buffer(size, '\0');
		file.seekg(0);
		if (file.read(&buffer[0], size))
		{
			return buffer;
		}

#ifndef NDEBUG
		if (file.eof())
		{
			std::cerr << "[fileio] Reached end of file unexpectedly while reading file " << filename << " with full path " << path << std::endl;
		}
		else if (file.bad())
		{
			std::cerr << "[fileio] A serious I/O error occurred while reading file " << filename << " with full path " << path << std::endl;
		}
		else if (file.fail())
		{
			std::cerr << "[fileio] Failed to read file " << filename << " with full path " << path << " due to a formatting or conversion issue" << std::endl;
		}
#endif

		assert(false);
		return {};
	}
}