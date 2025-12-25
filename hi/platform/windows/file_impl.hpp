#pragma once

#include "../../native/file.hpp"
#include "../../native/containers.hpp"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   undef CreateWindow
#   undef MessageBox
#endif

namespace io {
namespace native {

    // -------- UTF-8 <-> UTF-16 helpers --------

    static bool utf8_to_wide(io::char_view utf8, io::wstring& out) noexcept {
        out.clear();
        if (!utf8.data() || utf8.empty()) return false;
        if (utf8.size() > 0x7fffffff) return false; // int limit for WinAPI

        // need = wide chars WITHOUT NUL (`cause cbMultiByte != -1)
        int need = ::MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.data(), static_cast<int>(utf8.size()),
            nullptr, 0
        );
        if (need <= 0) return false;

        // resize(need) -> need+1
        // '\0' is guaranteed by io::wstring 
        if (!out.resize((usize)need)) return false;

        int wrote = ::MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.data(), static_cast<int>(utf8.size()),
            out.data(), need
        );
        return wrote == need;
    }

    struct file_handle {
        HANDLE h{ INVALID_HANDLE_VALUE };
        bool eof{ false };
        bool err{ false };
        io::OpenMode mode{ io::OpenMode::None };
    };

    IO_CONSTEXPR static DWORD access_from_mode(io::OpenMode m) noexcept {
        DWORD acc = 0;
        if (io::has(m, io::OpenMode::Read))  acc |= GENERIC_READ;
        if (io::has(m, io::OpenMode::Write) || io::has(m, io::OpenMode::Append)) acc |= GENERIC_WRITE;
        return acc;
    }

    IO_CONSTEXPR static DWORD disposition_from_mode(io::OpenMode m) noexcept {
        const bool rd = io::has(m, io::OpenMode::Read);
        const bool wr = io::has(m, io::OpenMode::Write) || io::has(m, io::OpenMode::Append);
        const bool tr = io::has(m, io::OpenMode::Truncate);
        const bool cr = io::has(m, io::OpenMode::Create);

        if (wr) {
            if (tr) return CREATE_ALWAYS;          // truncate (and create)
            if (cr) return OPEN_ALWAYS;            // create if missing
            return OPEN_EXISTING;                  // must exist
        }
        // read-only
        (void)rd;
        return OPEN_EXISTING;
    }

    inline bool open_file(file_handle*& out, io::char_view utf8_path, io::OpenMode mode) noexcept {
        out = nullptr;
        if (!utf8_path.data() || utf8_path.empty()) return false;

        io::wstring wpath;
        if (!utf8_to_wide(utf8_path, wpath)) return false;

        file_handle* fh = new file_handle{};
        if (!fh) return false;

        const DWORD access = access_from_mode(mode);
        const DWORD disp = disposition_from_mode(mode);

        IO_CONSTEXPR_VAR DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        IO_CONSTEXPR_VAR DWORD flags = FILE_ATTRIBUTE_NORMAL;

        HANDLE h = ::CreateFileW(
                       /* lpFileName */ wpath.c_str(),
                  /* dwDesiredAccess */ access,
                      /* dwShareMode */ share,
             /* dwSecurityAttributes */ nullptr,
            /* dwCreationDisposition */ disp,
             /* dwFlagsAndAttributes */ flags,
                    /* hTemplateFile */ nullptr
        );
        if (h == INVALID_HANDLE_VALUE) {
            delete fh;
            return false;
        }
        fh->h = h;
        fh->mode = mode;
        fh->eof = false;
        fh->err = false;

        // if Append — immediately to the end
        if (io::has(mode, io::OpenMode::Append)) {
            LARGE_INTEGER li;
            li.QuadPart = 0;
            if (!::SetFilePointerEx(fh->h, li, nullptr, FILE_END)) {
                fh->err = true;
            }
        }

        out = fh;
        return true;
    }

    inline void close_file(file_handle* h) noexcept {
        if (!h) return;
        if (h->h != INVALID_HANDLE_VALUE) {
            ::CloseHandle(h->h);
            h->h = INVALID_HANDLE_VALUE;
        }
        delete h;
    }

    IO_NODISCARD inline usize read_file(file_handle* h, void* dst, usize bytes) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE || !dst || bytes == 0) return 0;
        h->eof = false;

        DWORD got = 0;
        BOOL ok = ::ReadFile(h->h, dst, (DWORD)bytes, &got, nullptr);
        if (!ok) {
            h->err = true;
            return 0;
        }

        if (got == 0) {
            // for the files it's typically EOF
            h->eof = true;
        }
        return (usize)got;
    }

    IO_NODISCARD inline usize write_file(file_handle* h, const void* src, usize bytes) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE || !src || bytes == 0) return 0;
        h->eof = false;

        DWORD wrote = 0;
        BOOL ok = ::WriteFile(h->h, src, (DWORD)bytes, &wrote, nullptr);
        if (!ok) {
            h->err = true;
            return 0;
        }
        return (usize)wrote;
    }

    IO_NODISCARD inline bool flush_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return false;
        if (!::FlushFileBuffers(h->h)) {
            h->err = true;
            return false;
        }
        return true;
    }

    IO_NODISCARD inline bool seek_file(file_handle* h, i64 offset, io::SeekWhence whence) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return false;

        DWORD move = FILE_BEGIN;
        if (whence == io::SeekWhence::Current) move = FILE_CURRENT;
        else if (whence == io::SeekWhence::End) move = FILE_END;

        LARGE_INTEGER li;
        li.QuadPart = offset;

        BOOL ok = ::SetFilePointerEx(h->h, li, nullptr, move);
        if (!ok) {
            h->err = true;
            return false;
        }

        h->eof = false;
        return true;
    }

    IO_NODISCARD inline u64 tell_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return 0;

        LARGE_INTEGER zero;
        zero.QuadPart = 0;
        LARGE_INTEGER cur;

        BOOL ok = ::SetFilePointerEx(h->h, zero, &cur, FILE_CURRENT);
        if (!ok) {
            h->err = true;
            return 0;
        }
        return (u64)cur.QuadPart;
    }

    IO_NODISCARD inline u64 size_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return 0;

        LARGE_INTEGER sz;
        if (!::GetFileSizeEx(h->h, &sz)) {
            h->err = true;
            return 0;
        }
        return (u64)sz.QuadPart;
    }

    IO_NODISCARD inline bool is_eof(file_handle* h) noexcept { return h ? h->eof : true; }
    IO_NODISCARD inline bool has_error(file_handle* h) noexcept { return h ? h->err : true; }
    inline void clear_error(file_handle* h) noexcept { if (h) { h->err = false; h->eof = false; } }

} // namespace native
} // namespace io
