#pragma once

#include "types.hpp"
#include "containers.hpp" // io::string, io::wstring
#include "../filesystem.hpp"

namespace io {

    enum class SeekWhence : u8 {
        Begin,
        Current,
        End
    };

    enum class OpenMode : u8 {
        None = 0,
        Read = 1u << 0,
        Write = 1u << 1,
        Append = 1u << 2, // write at the end
        Truncate = 1u << 3, // truncate on open
        Create = 1u << 4, // Create if not exists
        Binary = 1u << 5, // for Win/Unix there's almost no difference
        Text = 1u << 6  // Optionally (immutable state)
    };

    IO_NODISCARD IO_CONSTEXPR OpenMode operator|(OpenMode a, OpenMode b) noexcept {
        return static_cast<OpenMode>(static_cast<u8>(a) | static_cast<u8>(b));
    }
    IO_NODISCARD IO_CONSTEXPR OpenMode operator&(OpenMode a, OpenMode b) noexcept {
        return static_cast<OpenMode>(static_cast<u8>(a) & static_cast<u8>(b));
    }
    IO_CONSTEXPR OpenMode& operator|=(OpenMode& a, OpenMode b) noexcept { a = a | b; return a; }
    IO_CONSTEXPR OpenMode& operator&=(OpenMode& a, OpenMode b) noexcept { a = a & b; return a; }

    IO_NODISCARD IO_CONSTEXPR bool has(OpenMode m, OpenMode f) noexcept {
        return (static_cast<u8>(m) & static_cast<u8>(f)) != 0;
    }

    namespace native {
        struct file_handle; // opaque; impl in platform/.../file_impl.hpp

        IO_NODISCARD bool  open_file(file_handle*& out, io::char_view utf8_path, OpenMode mode) noexcept;
        void               close_file(file_handle* h) noexcept;

        IO_NODISCARD usize read_file(file_handle* h, void* dst, usize bytes) noexcept;   // 0 => eof or error
        IO_NODISCARD usize write_file(file_handle* h, const void* src, usize bytes) noexcept;

        IO_NODISCARD bool  flush_file(file_handle* h) noexcept;

        IO_NODISCARD bool  seek_file(file_handle* h, i64 offset, SeekWhence whence) noexcept;
        IO_NODISCARD u64   tell_file(file_handle* h) noexcept;

        IO_NODISCARD u64   size_file(file_handle* h) noexcept;

        IO_NODISCARD bool  is_eof(file_handle* h) noexcept;
        IO_NODISCARD bool  has_error(file_handle* h) noexcept;
        void               clear_error(file_handle* h) noexcept;
    } // namespace native





    // ------------------------------------------------------------
    //                      io::File — RAII
    // ------------------------------------------------------------
    struct File {
        File() noexcept = default;
        inline explicit File(io::char_view path_utf8, OpenMode mode = OpenMode::Read) noexcept : _mode{ mode } { io::native::open_file(_h, path_utf8, _mode); }
        inline explicit File(const io::string& path_utf8, OpenMode mode = OpenMode::Read) noexcept : File(path_utf8.as_view(), mode) {}
        
        inline ~File() noexcept { close(); }

        File(const File&) = delete;
        File& operator=(const File&) = delete;

        File(File&& o) noexcept { *this = static_cast<File&&>(o); }
        File& operator=(File&& o) noexcept {
            if (this == &o) return *this;
            close();
            _h = o._h;        o._h = nullptr;
            _mode = o._mode;  o._mode = OpenMode::None;
            return *this;
        }

        // ---- lifetime ----
        IO_NODISCARD bool open(io::char_view path_utf8, OpenMode mode) noexcept {
            close();
            _mode = mode;
            return native::open_file(_h, path_utf8, mode);
        }
        IO_NODISCARD inline bool open(const io::string& path_utf8, OpenMode mode) noexcept { return open(path_utf8.as_view(), mode); }

        void close() noexcept {
            if (_h) {
                native::close_file(_h);
                _h = nullptr;
            }
            _mode = OpenMode::None;
        }

        IO_NODISCARD bool is_open() const noexcept { return _h != nullptr; }

        // ---- status like fstream ----
        IO_NODISCARD bool good() const noexcept { return _h && !native::has_error(_h); }
        IO_NODISCARD bool fail() const noexcept { return !_h || native::has_error(_h); }
        IO_NODISCARD bool eof() const noexcept { return _h ? native::is_eof(_h) : true; }
        void clear() noexcept { if (_h) native::clear_error(_h); }

        // ---- core i/o ----
        IO_NODISCARD usize read(io::view<char> dst) noexcept {
            if (!dst || !_h  || !has(_mode, OpenMode::Read)) return 0;
            return native::read_file(_h, dst.data(), dst.size());
        }

        IO_NODISCARD usize write(io::char_view src) noexcept {
            if (!src || !_h) return 0;
            // allow Write or Append
            if (!has(_mode, OpenMode::Write) &&
                !has(_mode, OpenMode::Append))
                return 0;
            return native::write_file(_h, src.data(), src.size());
        }

        IO_NODISCARD bool flush() noexcept { return _h ? native::flush_file(_h) : false; }
        IO_NODISCARD bool seek(i64 offset, SeekWhence whence) noexcept { return _h ? native::seek_file(_h, offset, whence) : false; }
        IO_NODISCARD u64 tell() const noexcept { return _h ? native::tell_file(_h) : 0; }
        IO_NODISCARD u64 size() const noexcept { return _h ? native::size_file(_h) : 0; }

        // ---- convenience helpers ----

        IO_NODISCARD bool write_str(io::char_view v) noexcept { return write(v) == v.size(); }
        IO_NODISCARD bool write_line(io::char_view line) noexcept {
            if (write(line) != line.size()) return false;
            return write({ "\n", 1 }) == 1;
        }

        // reads line till '\n'; return `false` if empty, or EOF
        IO_NODISCARD bool read_line(io::string& out) noexcept {
            out.clear();
            if (!_h) return false;

            char c = 0;
            io::view<char> one{ &c, 1 };
            bool got_any = false;

            for (;;) {
                if (read(one) == 0) break; // eof or error
                got_any = true;

                if (c == '\n') break;

                if (c == '\r') {
                    // peek 1 byte, undo if not '\n'
                    u64 pos_after_cr = tell(); // Windows CRLF: if next is '\n' - read
                    if (read(one) == 1) {
                        if (c != '\n')
                            seek(static_cast<i64>(pos_after_cr), SeekWhence::Begin);
                    }
                    break;
                }
                if (!out.push_back(c)) return false;
            }

            if (!got_any && eof()) return false;
            return got_any || !out.empty();
        }

        // read whole file into string
        IO_NODISCARD bool read_all(io::string& out) noexcept {
            out.clear();
            if (!_h) return false;
            u64 sz64 = size();

            // 1) known size: 1 alloc, 1 loop
            if (sz64 != 0) {
                if (sz64 > (u64)static_cast<usize>(-1)) return false;
                const usize sz = static_cast<usize>(sz64);

                if (!out.resize(sz)) return false;

                usize got = 0;
                while (got < sz) {
                    io::view<char> chunk{ out.data() + got, sz - got };
                    usize r = read(chunk);
                    if (r == 0) break;
                    got += r;
                }

                if (got < sz)
                    if (!out.resize(got)) return false;
                return good() || eof();
            }

            // 2) unknown size: chunked
            char buf[4096];
            io::view<char> chunk{ buf, sizeof(buf) };
            for (;;) {
                usize r = read(chunk);
                if (r == 0) break;
                if (!out.append(io::char_view{ buf, r })) return false;
            }
            return good() || eof();
        } // read_all

    private:
        native::file_handle* _h{ nullptr };
        OpenMode _mode{ OpenMode::None };
    };

} // namespace io

// platform binding
#ifdef __linux__
// include "../platform/linux/file_impl.hpp"
#elif defined(_WIN32)
#   include "../platform/windows/file_impl.hpp"
#endif
