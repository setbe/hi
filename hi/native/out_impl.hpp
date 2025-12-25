#pragma once

#include "types.hpp"

// ---------------- Implementation of io::native::Out -----------------------

namespace io {
    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(out_buffer, char, IO_TERMINAL_BUFFER_SIZE)
    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(in_buffer, char, IO_TERMINAL_BUFFER_SIZE)

namespace native {
#if defined(IO_TERMINAL) && defined(_WIN32)
    // ---------------- UTF-8 chunking helpers ----------------
    // Avoid splitting UTF-8 sequence at end of chunk.
    // Returns chunk length <= max_bytes and > 0 if rem > 0.
    static inline usize utf8_safe_chunk_len(const char* s, usize rem, usize max_bytes) noexcept {
        if (!s || rem == 0) return 0;
        if (max_bytes == 0) return 0;
    
        usize n = rem < max_bytes ? rem : max_bytes;
        if (n == rem) return n; // whole remainder fits
    
        // If last byte is not a continuation, safe.
        auto is_cont = [](unsigned char c) -> bool { return (c & 0xC0u) == 0x80u; };
    
        // If the chunk ends on a continuation byte, back up to the lead byte.
        usize end = n;
        while (end > 0 && is_cont(
            static_cast<unsigned char>(s[end - 1]))) {
            --end;
        }
    
        // If end == n, we are not ending on a continuation -> safe
        if (end == n) return n;
    
        // If end == 0, we couldn't find a lead byte inside the chunk.
        // Fallback: emit 1 byte (will likely be invalid, but prevents infinite loop).
        if (end == 0) return 1;
    
        // Now s[end-1] is a non-cont byte, it might be ASCII or a lead byte.
        unsigned char lead = static_cast<unsigned char>(s[end - 1]);
    
        // Determine UTF-8 sequence length from lead byte.
        usize seq_len = 1;
        if ((lead & 0x80u) == 0x00u) seq_len = 1; // ASCII
        else if ((lead & 0xE0u) == 0xC0u) seq_len = 2;
        else if ((lead & 0xF0u) == 0xE0u) seq_len = 3;
        else if ((lead & 0xF8u) == 0xF0u) seq_len = 4;
        else // Invalid lead; safest: don't include it if it would be split.
            return end - 1 > 0 ? (end - 1) : 1;
        // We currently include up to n bytes. Our last non-cont byte is at (end-1).
        // The bytes [end-1 .. end-1+seq_len) must be fully inside the chunk.
        usize seq_start = end - 1;
        usize seq_end = seq_start + seq_len;
        if (seq_end <= n)
            // Even though there were continuation bytes, the last sequence fits fully.
            return n;
        // Otherwise, cut before the start of this partial sequence.
        return seq_start > 0 ? seq_start : 1;
    }
    
    // Write UTF-8 bytes to Windows console, chunked, using UTF-16 conversion.
    static inline void write_console_utf8_chunked(const char* msg, usize len) noexcept {
        if (!msg || len == 0) return;

        HANDLE hterminal = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hterminal == INVALID_HANDLE_VALUE || hterminal == nullptr) return;
    
        // Wide buffer: keep it reasonably sized.
        // NOTE: one UTF-8 codepoint can expand to 1-2 UTF-16 code units, so keep headroom.
        wchar_t wbuf[256];
    
        usize pos = 0;
        while (pos < len) {
            // Keep chunk smaller than wbuf to avoid overflow.
            // 200 bytes is safe-ish; you can tune this.
            const usize max_chunk = 200;
            usize rem = len - pos;
            usize chunk = utf8_safe_chunk_len(msg + pos, rem, max_chunk);
            if (chunk == 0) break;
    
            int wlen = ::MultiByteToWideChar(
                CP_UTF8,
                0,
                msg + pos,
                (int)chunk,
                wbuf,
                (int)(sizeof(wbuf) / sizeof(wbuf[0]))
            );
    
            if (wlen <= 0) {
                // Fallback: emit '?' for this chunk (avoid infinite loop)
                DWORD written = 0;
                wchar_t q = L'?';
                ::WriteConsoleW(hterminal, &q, 1, &written, nullptr);
                pos += chunk;
                continue;
            }
    
            DWORD written = 0;
            ::WriteConsoleW(hterminal, wbuf, (DWORD)wlen, &written, nullptr);
    
            pos += chunk;
        }
    }
#endif // IO_TERMINAL && _WIN32




    inline void Out::reset() noexcept { global::out_buffer[0] = '\0'; global::out_buffer_count = 0; }
 
    inline void Out::write(const char* msg, usize len) noexcept {
#ifndef IO_TERMINAL
        (void)msg; (void)len;
#elif defined(_WIN32)
        if (!msg || len == 0) return;
        write_console_utf8_chunked(msg, len);
#else
#   error "Not implemented"
#endif
    }

    inline void Out::flush() noexcept {
#ifndef IO_TERMINAL
        return;
#else
        using namespace global;
        if (out_buffer_count == 0) {
            // Keep the invariant: zero-terminated buffer
            out_buffer[0] = '\0';
            return;
        }
        out_buffer[out_buffer_count] = '\0';
        write(out_buffer, out_buffer_count);
        reset();
#endif
    }

    inline io::char_view Out::scrap_view() const noexcept {
        using namespace global;
        // Ensure NUL-termination
        if (out_buffer_count >= IO_TERMINAL_BUFFER_SIZE)
            out_buffer_count = IO_TERMINAL_BUFFER_SIZE - 1;
        out_buffer[out_buffer_count] = '\0';
        return io::char_view{ out_buffer, out_buffer_count };
    }

    inline void Out::put(char c) noexcept {
#ifndef IO_TERMINAL
        (void)c;
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
        (void)str; (void)count;
#else
        using namespace global;
        while (count-- && out_buffer_count < IO_TERMINAL_BUFFER_SIZE - 1)
            out_buffer[out_buffer_count++] = *str++;
#endif
    }

    inline void Out::write_str(const char* str) noexcept {
#ifndef IO_TERMINAL
        (void)str;
#else
        using namespace global;
        while (*str && out_buffer_count < IO_TERMINAL_BUFFER_SIZE - 1)
            out_buffer[out_buffer_count++] = *str++;
#endif
    }

    inline void Out::write_unsigned(u64 v) noexcept {
#if !defined(IO_TERMINAL)
        (void)v;
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
    // --- primitives ---
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

    // --- complex types ---
    inline const Out& Out::operator<<(io::char_view v) const noexcept { write(v.data(), v.size()); return *this; }
    inline const Out& Out::operator<<(const ::io::string& v) const noexcept { return *this << v.as_view(); }
    inline const Out& Out::operator<<(const ::io::wstring& w) const noexcept {
#ifndef IO_TERMINAL
        (void)w; return *this;
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

    inline const Out& Out::operator<<(const Out& (*manip)(const Out&)) const noexcept { return manip(*this); }
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
