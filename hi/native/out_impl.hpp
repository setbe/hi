#pragma once

#include "types.hpp"

// ---------------- Implementation of io::native::Out -----------------------

namespace io {
    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(out_buffer, char, IO_TERMINAL_BUFFER_SIZE)
    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(in_buffer, char, IO_TERMINAL_BUFFER_SIZE)

namespace native {
 
    inline void Out::write(const char* msg, usize len) noexcept {
#ifndef IO_TERMINAL
        return;
#elif defined(_WIN32)
        if (len == 0) return;
        HANDLE hterminal = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hterminal == INVALID_HANDLE_VALUE) return;

        // Convert UTF-8 -> UTF-16
        wchar_t wbuf[IO_TERMINAL_BUFFER_SIZE];
        int wlen = ::MultiByteToWideChar(
            CP_UTF8, 0, msg, static_cast<int>(len),
            wbuf, sizeof(wbuf) / sizeof(wchar_t)
        );

        if (wlen > 0) {
            DWORD written;
            ::WriteConsoleW(hterminal, wbuf, (DWORD)wlen, &written, nullptr);
        }
#else
#   error "Not implemented"
#endif
    }

    inline void Out::flush() noexcept {
#ifndef IO_TERMINAL
        return;
#else
        using namespace global;
        out_buffer[out_buffer_count] = '\0';
        write(out_buffer, out_buffer_count);
        out_buffer_count = 0;
#endif
    }

    inline void Out::put(char c) noexcept {
#ifndef IO_TERMINAL
        return;
#else
        using namespace global;
        if (out_buffer_count < IO_TERMINAL_BUFFER_SIZE - 1)
            out_buffer[out_buffer_count++] = c;
        else {
            flush();
            out_buffer[out_buffer_count++] = c;
        }
#endif
    }

    inline void Out::write_str(const char* str, usize count) {
#ifndef IO_TERMINAL
        return;
#else
        using namespace global;
        while (count-- && out_buffer_count < IO_TERMINAL_BUFFER_SIZE - 1)
            out_buffer[out_buffer_count++] = *str++;
#endif
    }

    inline void Out::write_str(const char* str) noexcept {
#ifndef IO_TERMINAL
        return;
#else
        using namespace global;
        while (*str && out_buffer_count < IO_TERMINAL_BUFFER_SIZE - 1)
            out_buffer[out_buffer_count++] = *str++;
#endif
    }

    inline void Out::write_unsigned(u64 v) noexcept {
#if !defined(IO_TERMINAL)
        return;
#elif defined(IO_NOSTD) && defined(_MSC_VER) && defined(_M_IX86)
        // freestanding + MSVC x86: avoiding 64-bit division
        u32 x = static_cast<u32>(v); // cut to 32 bit

        char tmp[10]{};
        int i = 0;
        do {
            tmp[i++] = '0' + (x % 10u);  // 32-bit division
            x /= 10u;
        } while (x);
        while (--i >= 0) put(tmp[i]);
#else
        char tmp[20]{};
        int i = 0;
        do { tmp[i++] = '0' + (v % 10); v /= 10; } while (v);
        while (--i >= 0) put(tmp[i]);
#endif
    }

    inline void Out::write_signed(i64 v) noexcept {
#ifndef IO_TERMINAL
        return;
#else
        if (v < 0) { put('-'); write_unsigned(static_cast<u64>(-v)); }
        else                   write_unsigned(static_cast<u64>(v));
#endif
    }

    inline unsigned Out::pow10(int n) noexcept {
        unsigned r = 1;
        while (n--) r *= 10;
        return r;
    }

    inline void Out::write_float(double x, int precision) noexcept {
#if !defined(IO_TERMINAL)
        return;
#elif defined(IO_NOSTD) && defined(IO_MICROSHIT_NOSTD) && defined(_M_IX86)
        // In X86 Freestanding we DON'T print floats
        // avoiding call to _ftol2 and CRT.
        (void)x; (void)precision;
        write_str("[float]");
        return;
#else
        if (x < 0) { put('-'); x = -x; }

        int ip = static_cast<int>(x);
        double fp = x - ip;

        write_unsigned(ip);
        put('.');

        fp *= pow10(precision);
        write_unsigned(static_cast<u64>(fp + 0.5));
#endif
    }

    // --------------------------------------------------------------------------

    inline const Out& Out::operator<<(const char* s) const noexcept { write_str(s); return *this; }
    inline const Out& Out::operator<<(const unsigned char* s) const noexcept { return *this << reinterpret_cast<const char*>(s); }
    inline const Out& Out::operator<<(char c) const noexcept { put(c); return *this; }
    inline const Out& Out::operator<<(double v) const noexcept { write_float(v); return *this; }
    inline const Out& Out::operator<<(bool b) const noexcept { write_str(b ? "true" : "false"); return *this; }
    inline const Out& Out::operator<<(i8 v)  const noexcept { return *this << static_cast<i64>(v); }
    inline const Out& Out::operator<<(i16 v) const noexcept { return *this << static_cast<i64>(v); }
    inline const Out& Out::operator<<(i32 v) const noexcept { return *this << static_cast<i64>(v); }
    inline const Out& Out::operator<<(long v) const noexcept { return *this << static_cast<isize>(v); }
    inline const Out& Out::operator<<(i64 v) const noexcept { write_signed(v); return *this; }
    inline const Out& Out::operator<<(u8 v)  const noexcept { return *this << static_cast<u64>(v); }
    inline const Out& Out::operator<<(u16 v) const noexcept { return *this << static_cast<u64>(v); }
    inline const Out& Out::operator<<(u32 v) const noexcept { return *this << static_cast<u64>(v); }
    inline const Out& Out::operator<<(unsigned long v) const noexcept { return *this << static_cast<usize>(v); }
    inline const Out& Out::operator<<(u64 v) const noexcept { write_unsigned(v); return *this; }

    inline const Out& Out::operator<<(const ::io::string& v) const noexcept { write(v.data(), v.size()); return *this; }
    inline const Out& Out::operator<<(const ::io::wstring& w) const noexcept {
#ifndef IO_TERMINAL
        return *this;
#elif defined(_WIN32)
        if (w.size() == 0) return *this;
        HANDLE hterminal = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hterminal == INVALID_HANDLE_VALUE) return *this;
        DWORD written;
        ::WriteConsoleW(hterminal, w.data(), (DWORD)w.size(), &written, nullptr);
        return *this;
#else
#   error "Not implemented"
#endif
    }

    inline const Out& Out::operator<<(const Out& (*manip)(const Out&)) const noexcept {
        return manip(*this);
    }
    // ---------------- Hex printer ----------------

    inline const Out& Out::HexPrinter::operator()(const Out& o) const noexcept {
#ifndef IO_TERMINAL
        return o;
#else
        IO_CONSTEXPR_VAR char digits[] = "0123456789abcdef";
        const auto* p = static_cast<const u8*>(data);
        for (usize i = 0; i < size; ++i) {
            o.put(digits[(p[i] >> 4) & 0x0F]);
            o.put(digits[p[i] & 0x0F]);
        }
        return o;
#endif
    }
    inline Out::HexPrinter Out::hex(const void* d, usize s) noexcept { return { d, s }; }

    // ---------------- Str printer ----------------

    inline const Out& Out::StrPrinter::operator()(const Out& o) const noexcept {
#ifndef IO_TERMINAL
        return o;
#else
        for (usize i = 0; i < size; ++i)
            o.put(static_cast<char>(data[i]));
        return o;
#endif
    }
    inline Out::StrPrinter Out::str(const u8* d, usize s) noexcept { return { d, s }; }

    // ---------------- endl ----------------

    inline const Out& Out::endl(const Out& o) noexcept {
#ifndef IO_TERMINAL
        return o;
#else
        o.put('\n');
        o.flush();
        return o;
#endif
    }

    // ---------------- Input ----------------

    inline usize In::read(view<char> v) const noexcept {
#ifndef IO_TERMINAL
        return 0;
#elif defined(_WIN32)
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

        wchar_t wbuf[256];
        DWORD read = 0;

        if (!ReadConsoleW(h, wbuf, 255, &read, nullptr))
            return 0;

        // remove CR LF
        while (read > 0 && (wbuf[read - 1] == L'\n' || wbuf[read - 1] == L'\r'))
            read--;

        int bytes = WideCharToMultiByte(CP_UTF8, 0,
            wbuf, read,
            v.data(), static_cast<int>(v.size()) - 1,
            nullptr, nullptr);

        v[bytes] = 0;
        return bytes;
#elif defined(__linux__)
#       error "Not implemented"
#   else
#       error "Not implemented"
#endif
    }

} // namespace native
} // namespace io
