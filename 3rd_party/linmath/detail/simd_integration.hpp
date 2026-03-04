#pragma once

#if defined(LMATH_FORCE_NO_SIMD)
#  undef __SSE2__
#  undef __AVX__
#  undef __AVX2__
#  undef __ARM_NEON
#endif

#include <cstdint>

#if defined(_MSC_VER)
#   include <intrin.h>
#   if defined(__AVX__) || defined(__AVX2__)
#       include <immintrin.h>
#   endif
#endif


namespace lm {
namespace simd {

    enum class level : uint8_t {
        none = 0,
        sse2,
        avx,
        avx2,
        neon,
    };

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86)
    static inline void cpuid(
        uint32_t leaf,
        uint32_t subleaf,
        uint32_t& eax,
        uint32_t& ebx,
        uint32_t& ecx,
        uint32_t& edx
    ) noexcept
    {
#if defined(_MSC_VER)
        int regs[4];
        __cpuidex(regs, (int)leaf, (int)subleaf);
        eax = (uint32_t)regs[0];
        ebx = (uint32_t)regs[1];
        ecx = (uint32_t)regs[2];
        edx = (uint32_t)regs[3];
#else
        __asm__ volatile(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(leaf), "c"(subleaf)
            );
#endif
    }

    static inline uint64_t xgetbv(uint32_t index) noexcept
    {
#if defined(_MSC_VER)
        return ::_xgetbv(index);
#else
        uint32_t lo, hi;
        __asm__ volatile(
            "xgetbv"
            : "=a"(lo), "=d"(hi)
            : "c"(index)
            );
        return (uint64_t(hi) << 32) | lo;
#endif
#endif
    }


    inline level runtime_level() noexcept {
#if defined(__x86_64__) || defined(__i386__) || \
    defined(_M_IX86)    || defined(_M_X64)

            uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

            // --- basic features ---
            cpuid(1, 0, eax, ebx, ecx, edx);

            const bool hw_sse2 = (edx & (1u << 26)) != 0;
            const bool hw_avx = (ecx & (1u << 28)) != 0;
            const bool osxsave = (ecx & (1u << 27)) != 0;

#if defined(__AVX__) || defined(_MSC_VER)
            if (hw_avx && osxsave) {
                const uint64_t xcr0 = xgetbv(0);
                const bool ymm_enabled = (xcr0 & 0x6) == 0x6;

                if (ymm_enabled) {

#if defined(__AVX2__)
                    cpuid(7, 0, eax, ebx, ecx, edx);
                    if ((ebx & (1u << 5)) != 0)
                        return level::avx2;
#endif
                    return level::avx;
                }
            }
#endif

#if defined(__SSE2__) || defined(_MSC_VER)
            if (hw_sse2)
                return level::sse2;
#endif

            return level::none;

#elif defined(__aarch64__) || defined(__ARM_NEON)
            return level::neon;
#else
            return level::none;
#endif
    } // runtime_level

    inline level& max_level() noexcept {
        static level lvl = runtime_level();
        return lvl;
    }

} // namespace simd
} // namespace lm