#pragma once
#include "types.hpp"
#include "containers.hpp"

// -------------------- Macro Generators ------------------------------------
#ifndef IO_TERMINAL_BUFFER_SIZE
#   define IO_TERMINAL_BUFFER_SIZE 512
#endif

#if defined(_CONSOLE) && !defined(IO_TERMINAL)
#   define IO_TERMINAL
#endif

#ifdef IO_TERMINAL
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

#if defined(_WIN32) && !defined(IO_TERMINAL)
#define main() __stdcall WinMain( HINSTANCE hInstance,                        \
                                  HINSTANCE hPrevInstance,                    \
                                  LPSTR     lpCmdLine,                        \
                                  int       nCmdShow)
#endif

namespace io {
    
    namespace native {

        // ----------------------------------
        //            native::Out
        // ----------------------------------
        struct Out {
            explicit inline Out() noexcept {}
            inline ~Out() noexcept {}

            static inline void write(const char* msg, usize len) noexcept;
            static inline void flush() noexcept;

        protected:
            static inline void put(char c) noexcept;
            static inline void write_str(const char* str, usize count);
            static inline void write_str(const char* str) noexcept;
            static inline void write_unsigned(u64 value) noexcept;
            static inline void write_signed(i64 value) noexcept;
            static inline void write_float(double v, int precision = 6) noexcept;
            static inline unsigned pow10(int n) noexcept;

        public:
            // --- primitive operators ---
            inline const Out& operator<<(double v) const noexcept;
            inline const Out& operator<<(bool b) const noexcept;
            inline const Out& operator<<(const char* s) const noexcept;
            inline const Out& operator<<(const unsigned char* s) const noexcept;
            inline const Out& operator<<(char c) const noexcept;
            inline const Out& operator<<(i8 v)  const noexcept;
            inline const Out& operator<<(i16 v) const noexcept;
            inline const Out& operator<<(i32 v) const noexcept;
            inline const Out& operator<<(long v) const noexcept;
            inline const Out& operator<<(i64 v) const noexcept;
            inline const Out& operator<<(u8 v)  const noexcept;
            inline const Out& operator<<(u16 v) const noexcept;
            inline const Out& operator<<(u32 v) const noexcept;
            inline const Out& operator<<(unsigned long v) const noexcept;
            inline const Out& operator<<(u64 v) const noexcept;

            // --- complex operators ---
            inline const Out& operator<<(const ::io::string& v) const noexcept;
            inline const Out& operator<<(const ::io::wstring& w) const noexcept;

            inline const Out& operator<<(const Out& (*manip)(const Out&)) const noexcept;

            
            struct HexPrinter { const void* data; usize size; const Out& operator()(const Out& o) const noexcept; };
            static inline HexPrinter hex(const void* data, usize size) noexcept;

            struct StrPrinter {  const u8* data; usize size; const Out& operator()(const Out& o) const noexcept; };
            static inline StrPrinter str(const u8* data, usize size) noexcept;

            static inline const Out& endl(const Out& o) noexcept;

        }; // struct Out

        // ----------------------------------
        //           native::In
        // ----------------------------------
        struct In {
            explicit inline In() noexcept {}
            inline ~In() noexcept {}

            inline usize read(view<char> v) const noexcept;
        }; // struct In
    } // namespace native

    static native::Out out;
    static native::In in;

    

} // namespace io

// external operator>> for native::In
inline io::native::In& operator>>(io::native::In& in_obj, io::view<char> v) noexcept {
    if (!v.data() || v.size() == 0) return in_obj;

    usize n = in_obj.read(v);   // read into buffer
    if (n < v.size()) {
        v[n] = 0;               // null-terminate
    }
    return in_obj;
}

#include "out_impl.hpp"