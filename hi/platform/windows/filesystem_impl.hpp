#pragma once

#include "../../native/filesystem.hpp"
#include "../../native/out.hpp"
#include "../../native/containers.hpp"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   undef CreateWindow
#   undef MessageBox
#endif

namespace fs {
    namespace native {

        // -------- UTF-8 <-> UTF-16 helpers (RAII via io::wstring) --------

        static bool utf8_to_wide(io::char_view utf8, io::wstring& out) noexcept {
            out.clear();
            if (!utf8) return false;

            // required wchar count WITHOUT null terminator
            int need = ::MultiByteToWideChar(CP_UTF8, 0,
                utf8.data(), (int)utf8.size(),
                nullptr, 0);
            if (need <= 0) return false;

            // resize(need) => size() == need, storage includes '\0'
            if (!out.resize(static_cast<io::usize>(need))) return false;

            int wrote = ::MultiByteToWideChar(CP_UTF8, 0,
                utf8.data(), (int)utf8.size(),
                out.data(), need);
            return wrote == need;
        }

        static bool wide_to_utf8(const wchar_t* w, io::string& out) noexcept {
            out.clear();
            if (!w) return false;

            // required bytes including null terminator
            int need = ::WideCharToMultiByte(
                CP_UTF8, 0,
                w, -1,
                nullptr, 0,
                nullptr, nullptr
            );
            if (need <= 0) return false;

            // out.resize(need-1) => internal buffer size becomes need incl '\0'
            if (!out.resize(static_cast<io::usize>(need - 1))) return false;

            int wrote = ::WideCharToMultiByte(
                CP_UTF8, 0,
                w, -1,
                out.data(), need,
                nullptr, nullptr
            );
            return wrote > 0;
        }


        // -------- file_type mapping --------

        inline file_type from_attrs(DWORD attrs) noexcept {
            if (attrs == INVALID_FILE_ATTRIBUTES) return file_type::not_found;
            if (attrs & FILE_ATTRIBUTE_DIRECTORY) return file_type::directory;
            return file_type::regular;
        }

        // -------- dir_handle (no raw wchar_t*) --------

        struct dir_handle {
            HANDLE find{ INVALID_HANDLE_VALUE };
            WIN32_FIND_DATAW data{};
            bool first_pending{ false };

            io::wstring dir_prefix{};
            io::wstring search_pattern{};
        };

        static bool make_search_pattern(const io::wstring& dir, io::wstring& out_pat) noexcept {
            out_pat.clear();
            if (!out_pat.reserve(dir.size() + 3)) return false;
            if (!out_pat.append(dir.c_str())) return false;

            if (!dir.empty()) {
                wchar_t last = dir[dir.size() - 1];
                if (last != L'\\' && last != L'/') {
                    if (!out_pat.push_back(L'\\')) return false;
                }
            }
            if (!out_pat.push_back(L'*')) return false;
            return true;
        }

        // ---------------- API implementation ----------------

        inline bool stat(io::char_view utf8_path, file_type& out_type, io::u64* out_size) noexcept {
            if (!utf8_path) {
                out_type = file_type::unknown;
                if (out_size) *out_size = 0;
                return false;
            }

            io::wstring w;
            if (!utf8_to_wide(utf8_path, w)) {
                out_type = file_type::unknown;
                if (out_size) *out_size = 0;
                return false;
            }

            WIN32_FILE_ATTRIBUTE_DATA fad{};
            if (!::GetFileAttributesExW(w.c_str(), GetFileExInfoStandard, &fad)) {
                out_type = file_type::not_found;
                if (out_size) *out_size = 0;
                return true;
            }

            out_type = from_attrs(fad.dwFileAttributes);

            if (out_size) {
                ULARGE_INTEGER li;
                li.LowPart = fad.nFileSizeLow;
                li.HighPart = fad.nFileSizeHigh;
                *out_size = li.QuadPart;
            }
            return true;
        }

        inline bool create_directory(io::char_view utf8_path) noexcept {
            if (!utf8_path) return false;
            io::wstring w;
            if (!utf8_to_wide(utf8_path, w)) return false;
            return ::CreateDirectoryW(w.c_str(), nullptr) != 0;
        }

        inline bool remove(io::char_view utf8_path) noexcept {
            if (!utf8_path) return false;

            file_type ft{};
            io::u64 sz{};
            if (!stat(utf8_path, ft, &sz)) return false;

            io::wstring w;
            if (!utf8_to_wide(utf8_path, w)) return false;
            if (ft == file_type::directory)
                return ::RemoveDirectoryW(w.c_str()) != 0;
            return ::DeleteFileW(w.c_str()) != 0;
        }

        inline bool rename(io::char_view utf8_from, io::char_view utf8_to) noexcept {
            if (!utf8_from || !utf8_to) return false;

            io::wstring wfrom, wto;
            if (!utf8_to_wide(utf8_from, wfrom)) return false;
            if (!utf8_to_wide(utf8_to, wto)) return false;

            return ::MoveFileW(wfrom.c_str(), wto.c_str()) != 0;
        }

        inline bool current_directory(io::string& out_utf8) noexcept {
            out_utf8.clear();

            // 1) query required length (includes '\0')
            DWORD need = ::GetCurrentDirectoryW(0, nullptr);
            if (need == 0) return false;

            // 2) allocate wide buffer (size == need-1 characters, but storage includes '\0')
            io::wstring wdir;
            if (!wdir.resize(static_cast<io::usize>(need - 1))) return false;

            // 3) fill
            DWORD got = ::GetCurrentDirectoryW(need, wdir.data());
            if (got == 0) return false;
            // If path changed between calls, got may be >= need; handle by retry
            if (got >= need) {
                // got is required length excluding '\0' in some cases; safest: retry with got+1
                if (!wdir.resize(static_cast<io::usize>(got))) return false;
                DWORD got2 = ::GetCurrentDirectoryW(got + 1, wdir.data());
                if (got2 == 0 || got2 > got) return false;
            }

            // 4) convert to UTF-8
            return wide_to_utf8(wdir.c_str(), out_utf8);
        }

        inline dir_handle* open_dir(io::char_view utf8_path) noexcept {
            if (!utf8_path) return nullptr;

            dir_handle* h = new dir_handle{};
            if (!h) return nullptr;
            if (!utf8_to_wide(utf8_path, h->dir_prefix)) {
                delete h;
                return nullptr;
            }
            if (!make_search_pattern(h->dir_prefix, h->search_pattern)) {
                delete h;
                return nullptr;
            }
            h->find = ::FindFirstFileW(h->search_pattern.c_str(), &h->data);
            if (h->find == INVALID_HANDLE_VALUE) {
                delete h;
                return nullptr;
            }
            h->first_pending = true;
            return h;
        }

        inline bool read_dir(dir_handle* h,
            io::string& out_name,
            file_type& out_type,
            io::u64& out_size) noexcept
        {
            if (!h) return false;

            WIN32_FIND_DATAW* data = &h->data;

            if (!h->first_pending) {
                if (!::FindNextFileW(h->find, data))
                    return false;
            }
            else {
                h->first_pending = false;
            }
            // Convert filename to UTF-8 into io::string
            if (!wide_to_utf8(data->cFileName, out_name))
                out_name.clear(); // fallback empty
            out_type = from_attrs(data->dwFileAttributes);

            ULARGE_INTEGER li;
            li.LowPart = data->nFileSizeLow;
            li.HighPart = data->nFileSizeHigh;
            out_size = li.QuadPart;

            return true;
        }


        inline void close_dir(dir_handle* h) noexcept {
            if (!h) return;
            if (h->find != INVALID_HANDLE_VALUE) ::FindClose(h->find);
            delete h;
        }
    } // namespace native
} // namespace fs