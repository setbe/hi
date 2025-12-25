#pragma once

#include "types.hpp"

namespace fs {
    namespace native {
        IO_CONSTEXPR_VAR io::usize MAX_PATH_LENGTH = 260;
        IO_CONSTEXPR_VAR io::usize MAX_NAME_LENGTH = 256;

        enum class file_type : unsigned char {
            none,
            not_found,
            regular,
            directory,
            symlink,
            other,
            unknown
        };

        struct dir_handle; // opaque, impl. in platform/.../filesystem_impl.hpp

        // Get type and size of a file/directory.
        IO_NODISCARD inline bool stat(io::char_view utf8_path, file_type& outType, io::u64* outSize) noexcept;
        IO_NODISCARD inline bool create_directory(io::char_view utf8_path) noexcept;
        
        // Delete EMPTY directory or a file
        IO_NODISCARD inline bool remove(io::char_view utf8_path) noexcept;
        IO_NODISCARD inline bool rename(io::char_view utf8_from, io::char_view utf8_to) noexcept;
        IO_NODISCARD inline bool current_directory(io::string& out_utf8) noexcept;

        // ----- Directory job -----

        // Returns nullptr on error.
        IO_NODISCARD inline dir_handle* open_dir(io::char_view utf8_path) noexcept;

        // Read next element of a dir.
        // Returns false, when no elements left or error occurred.
        IO_NODISCARD inline bool read_dir(dir_handle* h,
                                          io::string& outName,
                                           file_type& outType,
                                             io::u64& outSize) noexcept;
        inline void close_dir(dir_handle* h) noexcept;
    } // namespace native
} // namespace fs

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   include "../platform/windows/filesystem_impl.hpp"
#endif
