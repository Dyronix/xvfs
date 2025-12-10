#include "vfs.h"
#include "file_info.h"

#include "string/string_ops.h"

#include <fstream>
#include <unordered_map>
#include <iostream>

namespace vfs
{
	struct context
	{
		std::unordered_map<std::string, std::string> wildcards;
	} g_ctx;

	//-------------------------------------------------------------------------
	void add_wildcard(const std::string& wildcard, const std::string& value)
	{
		std::cout << "[VFS] Adding vfs wildcard: ["<< wildcard<<" | "<< value <<"]" << std::endl;

		g_ctx.wildcards[wildcard] = value;
	}

	//-------------------------------------------------------------------------
	std::string resolve_path(const std::string& filename)
	{
		std::string	resolved_path;

		// No wildcards available return empty string
		if (g_ctx.wildcards.empty())
		{
			std::cerr << "[VFS] No wildcards available to resolve path" << std::endl;
			return resolved_path;
		}

		// Heuristic: anything with a '.' in the last path segment is treated as a file.
    	const bool has_extension = filename.find_last_of('.') != std::string::npos;

		if (has_extension)
    	{
			// ---------------------- FILE CASE ----------------------
			file_info_view file_info_view(filename);

			for (const auto& p : g_ctx.wildcards)
			{
				std::string_view sv_wildcard = p.first;
				std::string_view sv_path = p.second;

				if (file_info_view.path().find(sv_wildcard) != std::string::npos)
				{
					resolved_path = string::string_replace<std::string>(file_info_view.path(), sv_wildcard, sv_path);
				}
			}

			return file_info::make_full_path(resolved_path.empty() ? file_info_view.path() : resolved_path, file_info_view.filename(), file_info_view.extension());
		}

		// ------------------- DIRECTORY CASE --------------------
		directory_info_view info(filename);

		for (const auto& p : g_ctx.wildcards)
		{
			std::string_view sv_wildcard = p.first;
			std::string_view sv_path     = p.second;

			if (info.path().find(sv_wildcard) != std::string::npos)
			{
				resolved_path = string::string_replace<std::string>(info.path(), sv_wildcard, sv_path);
			}
		}

		const std::string final_path = resolved_path.empty()
			? std::string(info.path())
			: resolved_path;

		return directory_info::make_full_path(final_path, info.name());
	}

	//-------------------------------------------------------------------------
	bool exists_filepath(const std::string& filename)
	{
		const std::string path = resolve_path(filename);

		std::ifstream f(path.c_str());
		bool good = f.good();
		f.close();

		return good;
	}
}