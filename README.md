# xvfs

A small C++20 library that turns content paths such as `content:/maps/harbor.map` into real
paths, and reads whole files through them. Game and tool code names files relative to a
wildcard; only the place that sets up the wildcards knows where the content really lives
(install folder, development checkout, save folder, a console's mounted drive).

- No global state: the caller owns an `xvfs::context`.
- No printing, asserts or exceptions: errors are returned; messages go to an optional callback.
- No dependencies beyond the standard library.
- MIT licence (see `LICENSE`). Version in `VERSION`, changes in `CHANGELOG.md`.

## Usage

```cpp
#include "xvfs/context.h"
#include "xvfs/fileio.h"

xvfs::context vfs;
vfs.set_log_callback([](xvfs::log_level level, std::string_view message) { my_log(level, message); });

if (vfs.add_wildcard("content:", "/home/me/game/content") != xvfs::error_code::none)
{
    // Only an empty wildcard is rejected.
}

// Resolve a path: one allocation, for the returned string.
const xvfs::result<std::string> path = vfs.resolve_path("content:/maps/harbor.map");
if (path.has_value())
{
    // path.get_value() == "/home/me/game/content/maps/harbor.map"
}

// Resolve without allocating, into memory you own.
std::array<char, 512> buffer;
const xvfs::result<std::string_view> view = vfs.resolve_path_into("content:/maps/harbor.map", buffer);

// Read a whole file. The allocator parameter is optional.
const xvfs::result<std::vector<std::byte>> bytes = xvfs::read_binary_file(vfs, "content:/maps/harbor.map");
if (!bytes.has_value())
{
    my_log_error(xvfs::get_error_message(bytes.get_error()));
}
```

## Wildcard rules

- A wildcard matches only at the **start** of a path. `content:` matches `content:/a.txt`,
  not `mods/content:/a.txt`.
- When several wildcards match, the **longest** wins, whatever the order they were added in.
  `content:dlc` beats `content:` for `content:dlc/pier.map`.
- The rest of the path is joined to the wildcard's directory with exactly **one** `/`, whether
  or not the directory ends in `/` and whether or not the path has a `/` after the wildcard.
  An empty directory gives a relative path (`content:/a.txt` becomes `a.txt`).
- A path that starts with no wildcard is returned unchanged, so absolute paths pass through.
- `\` and `/` are both accepted as separators, in paths and in wildcards. Every returned path
  uses `/`.
- Adding a wildcard that already exists replaces its directory. An empty wildcard is an error.
- Resolving with no wildcards at all is an error (`error_code::no_wildcards`), so a missing
  setup step is caught instead of silently reading from the working directory.

Tip: end every wildcard with a character that cannot start a directory name, such as `:`.
A wildcard `data` would also match `database/x.bin`.

## Paths are UTF-8

All path text in the API (wildcards, directories, content paths, resolved paths) is UTF-8 in
ordinary `char` strings. xvfs converts it explicitly wherever it touches the filesystem:

- UTF-8 text becomes a `std::filesystem::path` through `std::u8string`, so the standard library
  decodes it as UTF-8 on every platform: to UTF-16 on Windows, unchanged bytes on Linux and PS5.
  The active Windows code page plays no part, and the deprecated `std::filesystem::u8path` is
  not used.
- Text that is not well-formed UTF-8 (overlong forms, surrogates, code points above U+10FFFF,
  cut-off sequences) is rejected with `error_code::invalid_utf8` before any filesystem call,
  the same way on every platform.
- `resolve_path` is pure text: it passes bytes through and does not validate them. The check
  happens when a path reaches the filesystem (`read_*`, `open_input_file`, `is_file`,
  `is_directory`, `create_directories`).

Use `xvfs/native_path.h` at your own boundaries:

```cpp
// Hand a real directory to xvfs as UTF-8.
const std::string root = xvfs::make_utf8_path(std::filesystem::current_path());
const xvfs::error_code added = vfs.add_wildcard("local:", root);

// Open a resolved path with another library.
const xvfs::result<std::filesystem::path> native = xvfs::make_native_path(resolved_text);
```

`make_utf8_path` writes `/` separators. On Linux and PS5 it returns the bytes as they are, so a
file name that is not UTF-8 on disk stays that way.

## Threading

xvfs follows the standard library contract:

- `const` member functions of `xvfs::context` (`resolve_path`, `resolve_path_into`,
  `get_log_callback`) and every function that takes a `const xvfs::context&` (the reads, `is_file`,
  `is_directory`, `create_directories`) are safe to call from many threads at once.
- Non-const members (`add_wildcard`, `set_log_callback`) need exclusive access. Register every
  wildcard and the callback first, then share the context as `const xvfs::context&`.
- To change wildcards while other threads resolve (for example after mounting extra content),
  build a new context and swap it in at a point where no reads are in flight.
- The log callback can be called from several threads at once and must be thread safe.
- An `xvfs::input_file` belongs to one thread at a time.

There is no lock inside xvfs, so resolving costs no synchronization.

## Error handling

Every function that can fail returns `xvfs::error_code` (`none` means success) or
`xvfs::result<T>`, which holds either a value or an error code:

```cpp
xvfs::result<std::string> text = xvfs::read_text_file(vfs, "content:/config/game.json");
if (!text.has_value())
{
    return text.get_error();
}
std::string owned = std::move(text).get_value();
```

`get_value()` may only be called when `has_value()` is true (as with `std::optional`, this is a
documented precondition, not a runtime check). `xvfs::get_error_message` gives a short
sentence for logs. When a log callback is set, failures are also logged with the path involved.

Log levels: `debug` (a file was opened), `info` (a wildcard was added), `warning` (resolving
before any wildcard exists), `error` (a call failed; it also returns the error).

## Public headers

| Header | Contents |
|---|---|
| `xvfs/context.h` | `context`: wildcards, `resolve_path`, `resolve_path_into` |
| `xvfs/fileio.h` | `read_binary_file`, `read_text_file`, `is_file`, `is_directory`, `create_directories` |
| `xvfs/input_file.h` | `input_file`, `open_input_file`: read into memory you own |
| `xvfs/error.h` | `error_code`, `get_error_message` |
| `xvfs/result.h` | `result<T>` |
| `xvfs/log.h` | `log_level`, `log_callback` |
| `xvfs/file_info.h` | `file_info_view`, `file_info`, `make_file_path` |
| `xvfs/directory_info.h` | `directory_info_view`, `directory_info`, `make_directory_path` |
| `xvfs/path.h` | `make_normalized_path` |
| `xvfs/native_path.h` | `make_native_path`, `make_utf8_path`, `is_valid_utf8` |

## Building and testing

Needs CMake 3.21 or newer and a C++20 compiler (tested with Clang 18 and GCC 13).

```sh
cmake -S . -B out/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build out/debug
ctest --test-dir out/debug --output-on-failure
```

The tests use a small built-in runner, so nothing is downloaded at configure time.

| Option | Default | Effect |
|---|---|---|
| `XVFS_BUILD_TESTS` | on when top level | Builds `xvfs_tests` and registers it with CTest |
| `XVFS_INSTALL` | on when top level | Adds the install and package export rules |
| `XVFS_STRICT_WARNINGS` | on when top level | `-Wall -Wextra -Wpedantic` (`/W4` on MSVC) for xvfs's own targets |
| `XVFS_WARNINGS_AS_ERRORS` | off | `-Werror` (`/WX`) for xvfs's own targets |

The library is static. API documentation: run `doxygen` in the repository root; the HTML
lands in `out/docs/html`.

## Using xvfs in another project

Vendored (recommended): copy the repository, or add it as a submodule, and add it as a
subdirectory. Tests and install rules stay off in that case.

```cmake
add_subdirectory(third_party/xvfs)
target_link_libraries(my_game PRIVATE xvfs::xvfs)
```

Installed:

```sh
cmake --install out/debug --prefix /opt/xvfs
```

```cmake
find_package(xvfs 1.0 REQUIRED)
target_link_libraries(my_game PRIVATE xvfs::xvfs)
```

Include headers with their folder: `#include "xvfs/context.h"`.

## Notes

- File reads do no newline conversion: `read_text_file` returns the bytes as they are on disk.
