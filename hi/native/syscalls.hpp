#pragma once
#include "types.hpp"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#elif defined (__linux__)
    // linux includes here
#else
#   error "OS isn't specified"
#endif // WIN32


namespace io {
    namespace global {
#ifdef _WIN32
        static long long qpc_frequency{}; // needed for `monotonic_seconds()` on Windows
#endif // WIN32
    }

    // --- Allocate ---
    static inline void* alloc(usize bytes) noexcept {
#ifdef _WIN32
        return ::VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(__linux__)
        // store the allocated size in the allocated memory to bypass passing the allocated size to free(void*)
        usize total = bytes + sizeof(usize);
        void* base = ::mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) return nullptr;
        *reinterpret_cast<usize*>(base) = bytes;
        return reinterpret_cast<char*>(base) + sizeof(usize);
#else
#   error "Not implemented"
#endif
    }


    // --- Free ---
    static inline void free(void* ptr) noexcept {
        if (!ptr) return;
#ifdef _WIN32
        ::VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__linux__)
        void* base = reinterpret_cast<char*>(ptr) - sizeof(usize); // Recover base pointer
        usize bytes = *reinterpret_cast<usize*>(base);
        usize total = bytes + sizeof(usize);
        ::munmap(base, total);
#else
#   error "Not implemented"
#endif
    }


    // --- Exit Process ---
    static inline void exit_process(int error_code) noexcept {
#ifdef _WIN32
        ::ExitProcess(error_code);
#elif defined(__linux__)
        ::_exit(error_code); // async-signal-safe, avoids flushing I/O buffers
#else
#   error "Not implemented"
#endif
    }

    // --- Sleep ---
    static inline void sleep_ms(unsigned ms) noexcept {
#ifdef _WIN32
        ::Sleep(ms);
#elif defined(__linux__)
        ::usleep(static_cast<useconds_t>(ms) * 1000);
#else
#   error "Not implemented"
#endif
    }

    // --- Monotonic Timer ---
    static inline double monotonic_seconds() noexcept {
#ifdef _WIN32
        long long& freq = global::qpc_frequency;
        if (freq == 0) {
            LARGE_INTEGER f;
            ::QueryPerformanceFrequency(&f);
            freq = f.QuadPart;
        }
        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        return static_cast<double>(counter.QuadPart) / static_cast<double>(freq);
#elif defined(__linux__)
        struct timespec ts;
        ::clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;
#else
#   error "Not implemented"
#endif
    }

} // namespace io