#pragma once

#include <cstddef>
#include <cstdint>

#include "detail/feature_detection.hpp"
#include "detail/simd_integration.hpp"

#include "libc_integration.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#   include <xmmintrin.h>
#   include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#   include <arm_neon.h>
#endif

namespace lm {

    // ============================================================
    // Generic vector
    // ============================================================

    template<typename T, std::size_t N>
    struct vec {
        LMATH_CONSTEXPR T& operator[](std::size_t i)       noexcept { return v[i]; }
        LMATH_CONSTEXPR T  operator[](std::size_t i) const noexcept { return v[i]; }

        LMATH_CONSTEXPR       T* data()       noexcept { return v; }
        LMATH_CONSTEXPR const T* data() const noexcept { return v; }
        T v[N]{};
    };

    // ============================================================
    // Aliases
    // ============================================================

    template<typename T> using vec2_of = vec<T, 2>;
    template<typename T> using vec3_of = vec<T, 3>;
    template<typename T> using vec4_of = vec<T, 4>;

    using vec2 = vec2_of<float>;
    using vec3 = vec3_of<float>;
    using vec4 = vec4_of<float>;

    using ivec2 = vec2_of<int>;
    using ivec3 = vec3_of<int>;
    using ivec4 = vec4_of<int>;

    using uvec2 = vec2_of<unsigned>;
    using uvec3 = vec3_of<unsigned>;
    using uvec4 = vec4_of<unsigned>;


    namespace detail {

        // @TODO: replace `inline` with `force inline`
#if defined(__SSE2__)
        inline float dot_sse2(const float* a, const float* b) noexcept {
            __m128 va = _mm_loadu_ps(a);
            __m128 vb = _mm_loadu_ps(b);
            __m128 mul = _mm_mul_ps(va, vb);

            // horizontal add (SSE2-safe)
            __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
            __m128 sums = _mm_add_ps(mul, shuf);
            shuf = _mm_movehl_ps(shuf, sums);
            sums = _mm_add_ss(sums, shuf);

            return _mm_cvtss_f32(sums);
        }
#endif

#if defined(__ARM_NEON)
        inline float dot_neon(const float* a, const float* b) noexcept {
            float32x4_t va = vld1q_f32(a);
            float32x4_t vb = vld1q_f32(b);
            float32x4_t mul = vmulq_f32(va, vb);

            float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
            return vget_lane_f32(sum, 0) + vget_lane_f32(sum, 1);
        }
#endif

    } // namespace detail

    // ============================================================
    // vec common ops (generic)
    // ============================================================

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_add(const vec<T,N>& A,
                               const vec<T,N>& B) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i)
            res[i] = A[i]+B[i];
        return res;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_sub(const vec<T,N>& A,
                               const vec<T,N>& B) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i)
            res[i] = A[i]-B[i];
        return res;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_scale(const vec<T,N>& V,
                                               T S) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i)
            res[i] = V[i] * S;
        return res;
    }

    template<typename T, std::size_t N>
    LMATH_OUT T vec_dot(const vec<T,N>& A,
                        const vec<T,N>& B) noexcept {
        T res{};
        for (int i=0; i<N; ++i)
            res += A[i] * B[i];
        return res;
    }

    template<typename T, std::size_t N>
    LMATH_OUT T vec_len(const vec<T, N>& V) noexcept {
        return ::lm::sqrtf(vec_dot(V,V));
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_norm(const vec<T,N>& V) noexcept {
        const T LEN = vec_len(V);
        return LEN==T(0) ? vec<T,N>{}
                         : vec_scale(V, T(1)/LEN);
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_min(const vec<T,N>& A,
                               const vec<T,N>& B) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i)
            res[i] = A[i]<B[i] ? A[i] : B[i];
        return res;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> vec_max(const vec<T,N>& A,
                               const vec<T,N>& B) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i)
            res[i] = A[i]>B[i] ? A[i] : B[i];
        return res;
    }



    // ============================================================
    // overloaded operators
    // ============================================================

    //  + -
    
    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator+ (const vec<T,N>& A,
                                  const vec<T,N>& B) noexcept {
        return vec_add(A,B);
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator- (const vec<T, N>& A,
                                  const vec<T, N>& B) noexcept {
        return vec_sub(A, B);
    }

    // unary -

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator- (const vec<T,N>& V) noexcept {
        vec<T,N> res{};
        for (int i=0; i<N; ++i) res[i] = -V[i];
        return res;
    }

    // scalar *

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator* (const vec<T,N>& V,
                                                T S) noexcept {
        return vec_scale(V,S);
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator* (              T S,
                                  const vec<T,N>& V) noexcept {
        return vec_scale(V,S);
    }

    // scalar /

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> operator/ (const vec<T,N>& V,
                                                T S) noexcept {
        return vec_scale(V, T(1)/S);
    }


    // Compound operators (+= -= *= /=)

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N>& operator+= (      vec<T,N>& A,
                                    const vec<T,N>& B) noexcept {
        A = vec_add(A,B);
        return A;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N>& operator-= (      vec<T,N>& A,
                                    const vec<T,N>& B) noexcept {
        A = vec_sub(A,B);
        return A;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N>& operator*= (vec<T,N>& V,
                                            T S) noexcept {
        V = vec_scale(V,S);
        return V;
    }

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N>& operator/= (vec<T,N>& V,
                                            T S) noexcept {
        V = vec_scale(V, T(1)/S);
        return V;
    }


    // comparisons

    template<typename T, std::size_t N>
    LMATH_OUT bool operator== (const vec<T,N>& A,
                               const vec<T,N>& B) noexcept {
        for (int i=0; i<N; ++i) if (A[i] != B[i]) return false;
        return true;
    }

    template<typename T, std::size_t N>
    LMATH_OUT bool operator!= (const vec<T,N>& A, 
                               const vec<T,N>& B) noexcept {
        return !(A==B);
    }


    // ============================================================
    // vec3/vec4 cross and reflect remain generic for now
    // ============================================================

    template<typename T>
    LMATH_OUT vec3_of<T> vec3_cross(const vec3_of<T>& A, const vec3_of<T>& B) noexcept {
        return { A[1] * B[2] - A[2] * B[1],
                 A[2] * B[0] - A[0] * B[2],
                 A[0] * B[1] - A[1] * B[0] };
    }

    template<typename T>
    LMATH_OUT vec3_of<T> vec3_reflect(const vec3_of<T>& V, const vec3_of<T>& N) noexcept {
        const T P = T(2) * vec_dot(V, N);
        vec3_of<T> res{};
        for (int i = 0; i < 3; ++i)
            res[i] = V[i] - P * N[i];
        return res;
    }

    template<typename T>
    LMATH_OUT vec4_of<T> vec4_cross(const vec4_of<T>& A, const vec4_of<T>& B) noexcept {
        return { A[1] * B[2] - A[2] * B[1],
                 A[2] * B[0] - A[0] * B[2],
                 A[0] * B[1] - A[1] * B[0],
                 T(1) };
    }

    template<typename T>
    LMATH_OUT vec4_of<T> vec4_reflect(const vec4_of<T>& V, const vec4_of<T>& N) noexcept {
        const T P = T(2) * vec_dot(V, N);
        vec4_of<T> res{};
        for (int i = 0; i < 4; ++i)
            res[i] = V[i] - P * N[i];
        return res;
    }


    // ============================================================
    // SIMD specializations (runtime-detect)
    // ============================================================
    
    // SIMD specialization
    inline float vec4_dot(const vec4& A,
                          const vec4& B) noexcept {
        const float* a = A.v;
        const float* b = B.v;

        switch (simd::max_level()) {

#if defined(__ARM_NEON)
        case simd::level::neon:
            return detail::dot_neon(a, b);
#endif

#if defined(__SSE2__)
        case simd::level::sse2:
        case simd::level::avx:
        case simd::level::avx2:
            return detail::dot_sse2(a, b);
#endif

        default:
            return vec_dot(A, B); // call general without specialization
        } // switch
    } // vec_dot

} // namespace lm
