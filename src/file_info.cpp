#include "file_info.h"

namespace vfs
{
	namespace internal
	{
		//-------------------------------------------------------------------------
		char get_separator()
		{
			return '/';
		}

		//-------------------------------------------------------------------------
		std::string ensure_trailing_separator(std::string_view path)
		{
			std::string new_path(path.data(), path.size());

			char separator = get_separator();

			if (!path.empty() && path.back() != separator)
			{
				return new_path + separator;
			}

			return new_path;
		}
	}
}