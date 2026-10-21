# Changelog

All notable changes to xvfs. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/).

## [1.0.1] - 2026-10-21

### Fixed
- The changelog gives the release date of 1.0.0 instead of "unreleased". No code changes.

## [1.0.0] - 2026-10-21

First versioned release. Almost every public name changed; code written against the earlier,
unversioned sources has to be updated (see "Migrating" below).

### Breaking changes
- Namespace `vfs` (and `fileio`, `string`) is now `xvfs`. The short names collided easily
  (`string` especially) and did not match the package name.
- Headers moved from `include/*.h` to `include/xvfs/*.h`; include them as `"xvfs/context.h"`.
- The global wildcard table is gone. Create an `xvfs::context` and call its members:
  `vfs::add_wildcard` becomes `context::add_wildcard`, `vfs::resolve_path` becomes
  `context::resolve_path`.
- Errors are returned instead of printed or asserted: `add_wildcard` returns
  `xvfs::error_code`; `resolve_path`, `read_binary_file` and `read_text_file` return
  `xvfs::result<T>`.
- The reads take the context as their first argument: `xvfs::read_text_file(vfs, path)`.
- `vfs::exists_filepath` is now `xvfs::is_file(vfs, path)`.
- Accessors gained a `get_` prefix (`get_path`, `get_filename`, `get_extension`, `get_name`),
  and `full_path()` is now `make_full_path()`.
- The static `file_info::make_full_path(path, filename, extension)` and
  `directory_info::make_full_path(path, name)` are now the free functions
  `xvfs::make_file_path` and `xvfs::make_directory_path`. The one-argument `make_full_path`
  overloads are replaced by `xvfs::make_normalized_path`.
- The constructors of the path classes are `explicit`.
- Removed from the public API: `string::string_replace`, `parsed_path`, `parsed_directory`
  and `vfs::internal`.
- CMake target `vfs` is now `xvfs`, with the alias `xvfs::xvfs`.

### Behaviour changes
- A wildcard matches only at the start of a path. Before, it matched anywhere in the directory
  part and every occurrence was replaced, so a wildcard such as `data` could rewrite
  `mydata/a.txt`.
- When several wildcards match, the longest wins. Before, the winner depended on the iteration
  order of an `std::unordered_map`, which differs between standard libraries.
- A wildcard also matches when nothing or only a file name follows it: `content:` resolves to
  the content directory and `content:a.txt` to `<directory>/a.txt`. Before, both came back
  unchanged.
- Exactly one `/` joins the wildcard's directory and the rest of the path. Before, a directory
  ending in `/` gave `//`, and an empty directory made the wildcard look unmatched.
- `\` is a separator in input paths and wildcards, for matching and for splitting. Before, only
  `/` split paths, so `content:\maps\a.map` was not resolved.
- Resolving an empty path returns `error_code::empty_path` (before: an empty string).
  Resolving with no wildcards returns `error_code::no_wildcards` (before: an empty string and
  a line on `std::cerr`).
- `file_info` looks for the extension in the last segment only, and a name without a dot has
  an empty extension. Before, a dot in a directory name (`data.v2/readme`) or no dot at all gave
  wrong parts in release builds and an assert in debug builds.
- `is_file` is false for directories. `exists_filepath` returned true for a directory on Linux.
- A failed read returns an error code. Before, it returned an empty container (indistinguishable
  from an empty file) and asserted in debug builds. A missing file or a directory gives
  `error_code::not_a_file`.
- Path text is UTF-8 on every platform and is converted explicitly through `std::u8string`
  before any filesystem call. Before, narrow strings went straight to `std::ifstream`, which on
  Windows reads them in the active code page, so non-ASCII paths failed or opened the wrong
  file. Malformed UTF-8 now fails with `error_code::invalid_utf8`.
- Nothing is printed. Messages go to the optional log callback; a successful binary read is a
  `debug` message instead of a line on `std::cerr`.

### Added
- `xvfs/native_path.h`: `make_native_path`, `make_utf8_path` and `is_valid_utf8`, and the
  error code `error_code::invalid_utf8`.
- `xvfs::context::resolve_path_into`, which resolves into a caller buffer without allocating.
- `xvfs::log_callback` and `context::set_log_callback`.
- `xvfs::input_file` and `xvfs::open_input_file`, to read into memory the caller owns.
- `xvfs::is_directory` and `xvfs::create_directories`.
- An allocator instance argument on `read_binary_file` and `read_text_file`, for stateful
  allocators. The allocator template parameters stay.
- Thread-safety contract: const members can be called from many threads at once.
- CMake package: `find_package(xvfs)`, install rules, `VERSION` file, the options
  `XVFS_BUILD_TESTS`, `XVFS_INSTALL`, `XVFS_STRICT_WARNINGS`, `XVFS_WARNINGS_AS_ERRORS`.
- Tests (`ctest`), a README, this changelog, Doxygen comments and a `Doxyfile`.

### Migrating
| Before | Now |
|---|---|
| `#include "vfs.h"` | `#include "xvfs/context.h"` |
| `vfs::add_wildcard("local:", dir);` | `const xvfs::error_code e = vfs.add_wildcard("local:", dir);` |
| `std::string p = vfs::resolve_path(f);` | `xvfs::result<std::string> p = vfs.resolve_path(f);` |
| `vfs::exists_filepath(f)` | `xvfs::is_file(vfs, f)` |
| `fileio::read_text_file(f)` | `xvfs::read_text_file(vfs, f)` |
| `file_info_view(f).path()` | `xvfs::file_info_view(f).get_path()` |
