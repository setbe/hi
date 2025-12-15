#pragma once

#include "native/types.hpp"
#include "native/filesystem.hpp"

namespace fs {
    using file_type = native::file_type;

    struct path_view {
        const char* c_str; // UTF-8, zero-terminated
        IO_CONSTEXPR path_view() noexcept : c_str{ nullptr } {}
        IO_CONSTEXPR path_view(const char* s) noexcept : c_str{ s } {}
        IO_NODISCARD IO_CONSTEXPR bool empty() const noexcept { return !c_str || *c_str == '\0'; }
    };

    struct directory_entry {
        path_view   path;
        file_type   type;
        u64         size;
    };

    struct directory_iterator {
        directory_iterator() noexcept = default;
        explicit directory_iterator(path_view dir) noexcept {
            if (!dir.empty()) {
                _handle   = native::open_dir(dir.c_str);
                _dir_path = dir.c_str;
                _at_end = (_handle == nullptr);
                ++(*this); // read first element
            }
        }
        ~directory_iterator() noexcept {
            if (_handle) {
                native::close_dir(_handle);
                _handle = nullptr;
            }
        }
        
        directory_iterator(const directory_iterator&) = delete;
        directory_iterator& operator=(const directory_iterator&) = delete;
 
        directory_iterator(directory_iterator&& other) noexcept {
            *this = static_cast<directory_iterator&&>(other);
        }
        
        directory_iterator& operator=(directory_iterator&& other) noexcept {
            if (this != &other) {
                if (_handle) native::close_dir(_handle);
                _handle   = other._handle;
                _dir_path = other._dir_path;
                _entry    = other._entry;
                _at_end   = other._at_end;
                _name_storage = static_cast<io::string&&>(other._name_storage);

                other._handle   = nullptr;
                other._dir_path = nullptr;
                other._at_end   = true;
                other._entry    = {};
                other._name_storage.clear();
            }
            return *this;
        }

        directory_iterator& operator++() noexcept {
            if (!_handle || _at_end) { _at_end = true; return *this; }

            io::string name;
            file_type type{};
            u64 size{};

            for (;;) {
                if (!native::read_dir(_handle, name, type, size)) {
                    _at_end = true;
                    break;
                }

                if (name == "." || name == "..")
                    continue;

                _name_storage.clear();
                _name_storage.append(_dir_path);
                if (!_name_storage.empty()) {
                    char last = _name_storage[_name_storage.size() - 1];
                    if (last != '\\' && last != '/')
                        _name_storage.push_back('\\');
                }
                _name_storage.append(io::char_view{ name.c_str(), name.size() });

                _entry.path.c_str = _name_storage.c_str();
                _entry.type = type;
                _entry.size = size;
                break;
            }
            return *this;
        } // operator++

        IO_NODISCARD const directory_entry& operator*() const noexcept { return _entry; }
        IO_NODISCARD const directory_entry* operator->() const noexcept { return &_entry; }
        IO_NODISCARD bool is_end() const noexcept { return _at_end || !_handle; }

    private:
        native::dir_handle* _handle{ nullptr };
        const char* _dir_path{ nullptr };
        directory_entry _entry{};
        bool _at_end{ true };
        io::string _name_storage{};
    }; // struct directory_iterator

    // ---- simple abstractions over native API ----

    IO_NODISCARD inline bool exists(path_view p) noexcept {
        if (p.empty())
            return false;
        
        native::file_type t{};
        u64 size{};
        if (!native::stat(p.c_str, t, &size))
            return false;
        
        return t != native::file_type::not_found;
    }
    IO_NODISCARD inline file_type status(path_view p, u64* out_size = nullptr) noexcept {
        if (p.empty()) {
            if (out_size)
                *out_size = 0;
            return file_type::unknown;
        }

        native::file_type t{};
        u64 size{};
        if (!native::stat(p.c_str, t, &size))
            return file_type::unknown;
        
        if (out_size)
            *out_size = size;
        return t;
    }
    IO_NODISCARD inline bool is_directory(path_view p) noexcept {
        u64 size{};
        return status(p, &size) == file_type::directory;
    }
    IO_NODISCARD inline bool is_regular_file(path_view p) noexcept {
        u64 size{};
        return status(p, &size) == file_type::regular;
    }
    IO_NODISCARD inline u64 file_size(path_view p) noexcept {
        u64 size{};
        if (status(p, &size) == file_type::regular) return size;
        return 0;
    }
    IO_NODISCARD inline bool create_directory(path_view p) noexcept {
        return native::create_directory(p.c_str);
    }
    IO_NODISCARD inline bool remove(path_view p) noexcept {
        return native::remove(p.c_str);
    }
    IO_NODISCARD inline bool rename(path_view from, path_view to) noexcept {
        return native::rename(from.c_str, to.c_str);
    }

    IO_NODISCARD inline bool current_directory(io::string& out_utf8) noexcept {
        return native::current_directory(out_utf8);
    }
} // namespace fs