#pragma once

#include "native/types.hpp"
#include "native/filesystem.hpp"
#include "native/containers.hpp"

namespace fs {
    using file_type = native::file_type;

    struct directory_entry {
        io::char_view  path;
        io::u64        size;
        file_type      type;
    };

    struct directory_iterator {
        directory_iterator() noexcept = default;
        
        explicit directory_iterator(io::char_view dir) noexcept {
            if (!dir) { _at_end = true; return; }

            (void)_dir_storage.append(dir);

            // Build stable prefix once: "<dir>\" (or "/")
#ifdef _WIN32
            const char sep = '\\';
#else
            const char sep = '/';
#endif
            _prefix_storage.clear();
            (void)_prefix_storage.append(_dir_storage.as_view());
            if (!_prefix_storage.empty()) {
                char last = _prefix_storage[_prefix_storage.size() - 1];
                if (last != '\\' && last != '/') (void)_prefix_storage.push_back(sep);
            }
            else {
                (void)_prefix_storage.push_back(sep);
            }

            _handle = native::open_dir(_dir_storage.as_view());
            _at_end = (_handle == nullptr);
            if (!_at_end) ++(*this);
        }

        ~directory_iterator() noexcept {
            if (_handle) {
                native::close_dir(_handle);
                _handle = nullptr;
            }
        }
        
        directory_iterator(const directory_iterator&) = delete;
        directory_iterator& operator=(const directory_iterator&) = delete;

        directory_iterator(directory_iterator&& other) noexcept { *this = static_cast<directory_iterator&&>(other);}
        
        directory_iterator& operator=(directory_iterator&& o) noexcept {
            if (this == &o) return *this;
            if (_handle) native::close_dir(_handle);

            _handle = o._handle; o._handle = nullptr;
            _entry = o._entry;
            _at_end = o._at_end;

            _dir_storage    = static_cast<io::string&&>(o._dir_storage);
            _prefix_storage = static_cast<io::string&&>(o._prefix_storage);
            _name_storage   = static_cast<io::string&&>(o._name_storage);

            o._at_end = true;
            o._entry = {};
            o._dir_storage.clear();
            o._prefix_storage.clear();
            o._name_storage.clear();
            return *this;
        }

        directory_iterator& operator++() noexcept {
            if (!_handle || _at_end) { _at_end = true; return *this; }

            io::string name{};
            file_type type{};
            io::u64 size{};

            for (;;) {
                if (!native::read_dir(_handle, name, type, size)) {
                    _at_end = true;
                    _entry = {};
                    break;
                }

                if (name == "." || name == "..") continue;

                _name_storage.clear();
                (void)_name_storage.append(_prefix_storage.as_view());
                (void)_name_storage.append(name.as_view());

                _entry.path = _name_storage.as_view();
                _entry.type = type;
                _entry.size = size;
                break;
            }
            return *this;
        }

        IO_NODISCARD const directory_entry& operator*() const noexcept { return _entry; }
        IO_NODISCARD const directory_entry* operator->() const noexcept { return &_entry; }
        IO_NODISCARD bool is_end() const noexcept { return _at_end || !_handle; }

    private:
        native::dir_handle* _handle{ nullptr };
        directory_entry _entry{};
        bool _at_end{ true };

        io::string _dir_storage{};
        io::string _prefix_storage{}; // "<dir>\"
        io::string _name_storage{};   // "<dir>\file"
    }; // struct directory_iterator

    // ---- simple abstractions over native API ----

    IO_NODISCARD inline bool exists(io::char_view p) noexcept {
        if (!p) return false;
        file_type t{};
        io::u64 size{};
        if (!native::stat(p, t, &size)) return false;    
        return t != file_type::not_found;
    }
    IO_NODISCARD inline bool exists(const io::string& r) noexcept { return exists(r.as_view()); }

    IO_NODISCARD inline file_type status(io::char_view p, io::u64* out_size = nullptr) noexcept {
        if (!p) {
            if (out_size) *out_size = 0;
            return file_type::unknown;
        }
        file_type t{};
        io::u64 size{};
        if (!native::stat(p, t, &size)) return file_type::unknown;
        if (out_size) *out_size = size;
        return t;
    }
    IO_NODISCARD inline file_type status(const io::string& r, io::u64* out_size = nullptr) noexcept { return status(r.as_view()); }

    IO_NODISCARD inline bool is_directory(io::char_view p) noexcept { io::u64 size{}; return status(p, &size) == file_type::directory; }
    IO_NODISCARD inline bool is_directory(const io::string& r) noexcept { return is_directory(r.as_view()); }

    IO_NODISCARD inline bool is_regular_file(io::char_view p) noexcept { io::u64 size{}; return status(p, &size) == file_type::regular; }
    IO_NODISCARD inline bool is_regular_file(const io::string& r) noexcept { return is_regular_file(r.as_view()); }

    IO_NODISCARD inline io::u64 file_size(io::char_view p) noexcept { io::u64 size{}; if (status(p, &size) == file_type::regular) return size; return 0; }
    IO_NODISCARD inline io::u64 file_size(const io::string& r) noexcept { return file_size(r.as_view()); }

    IO_NODISCARD inline bool create_directory(io::char_view p) noexcept { return native::create_directory(p); }
    IO_NODISCARD inline bool create_directory(const io::string& r) noexcept { return create_directory(r.as_view()); }

    IO_NODISCARD inline bool remove(io::char_view p) noexcept { return native::remove(p); }
    IO_NODISCARD inline bool remove(const io::string& r) noexcept { return remove(r.as_view()); }

    IO_NODISCARD inline bool rename(io::char_view from, io::char_view to) noexcept { return native::rename(from, to); }
    IO_NODISCARD inline bool rename(const io::string& from, const io::string& to) noexcept { return rename(from.as_view(), to.as_view()); }

    IO_NODISCARD inline bool current_directory(io::string& out_utf8) noexcept { return native::current_directory(out_utf8); }
} // namespace fs