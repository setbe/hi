#pragma once

#include <cstddef>
#include <cstdint>

#include "detail/feature_detection.hpp"

#include "libc_integration.hpp"
#include "vec.hpp"

#if defined(__SSE2__)
#   include <emmintrin.h>
#endif

#if defined(__ARM_NEON)
#   include <arm_neon.h>
#endif

namespace lm {

    // ============================================================
    // Matrix types (column-major, OpenGL-compatible)
    // ============================================================

    template<typename T, std::size_t C, std::size_t R>
    struct mat {
        LMATH_CONSTEXPR       vec<T,R>& operator[](std::size_t c)       noexcept { return col[c]; }
        LMATH_CONSTEXPR const vec<T,R>& operator[](std::size_t c) const noexcept { return col[c]; }

        vec<T,R> col[C]{};
    };

    // ============================================================
    // Aliases
    // ============================================================

    template<typename T> using mat2x3_of = mat<T,2,3>;
    template<typename T> using mat3_of   = mat<T,3,3>;
    template<typename T> using mat4_of   = mat<T,4,4>;

    using mat2x3 = mat2x3_of<float>;
    using mat3   = mat3_of<float>;
    using mat4   = mat4_of<float>;


    namespace detail {

        // ============================================================
        // mat4 × vec4
        // ============================================================


        // @TODO: replace `inline` with `force inline`
#if defined(__SSE2__)
        inline ::lm::vec4 mat4_mul_vec_sse2(const ::lm::mat4& M,
            const ::lm::vec4& V) noexcept {
            __m128 vx = _mm_set1_ps(V[0]);
            __m128 vy = _mm_set1_ps(V[1]);
            __m128 vz = _mm_set1_ps(V[2]);
            __m128 vw = _mm_set1_ps(V[3]);

            __m128 c0 = _mm_loadu_ps(M[0].data());
            __m128 c1 = _mm_loadu_ps(M[1].data());
            __m128 c2 = _mm_loadu_ps(M[2].data());
            __m128 c3 = _mm_loadu_ps(M[3].data());

            __m128 r =
                _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(c0, vx), _mm_mul_ps(c1, vy)),
                    _mm_add_ps(_mm_mul_ps(c2, vz), _mm_mul_ps(c3, vw))
                );

            ::lm::vec4 out{};
            _mm_storeu_ps(out.v, r);
            return out;
        }
#endif


#if defined(__ARM_NEON)
        inline vec4 mat4_mul_vec_neon(const ::lm::mat4& M,
            const ::lm::vec4& V) noexcept {
            float32x4_t r =
                vmulq_n_f32(vld1q_f32(M[0].data()), V[0]);

            r = vmlaq_n_f32(r, vld1q_f32(M[1].data()), V[1]);
            r = vmlaq_n_f32(r, vld1q_f32(M[2].data()), V[2]);
            r = vmlaq_n_f32(r, vld1q_f32(M[3].data()), V[3]);

            ::lm::vec4 out{};
            vst1q_f32(out.v, r);
            return out;
        }
#endif


        // ============================================================
        // mat4 × mat4
        // ============================================================

#if defined(__SSE2__)
        inline ::lm::mat4 mat4_mul_sse2(const ::lm::mat4& A,
            const ::lm::mat4& B) noexcept {
            ::lm::mat4 R{};

            for (int c = 0; c < 4; ++c) {
                __m128 bx = _mm_set1_ps(B[c][0]);
                __m128 by = _mm_set1_ps(B[c][1]);
                __m128 bz = _mm_set1_ps(B[c][2]);
                __m128 bw = _mm_set1_ps(B[c][3]);

                __m128 r =
                    _mm_add_ps(
                        _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(A[0].data()), bx),
                                   _mm_mul_ps(_mm_loadu_ps(A[1].data()), by)),
                        _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(A[2].data()), bz),
                                   _mm_mul_ps(_mm_loadu_ps(A[3].data()), bw))
                    );

                _mm_storeu_ps(R[c].data(), r);
            }
            return R;
        }
#endif


#if defined(__ARM_NEON)
        inline ::lm::mat4 mat4_mul_neon(const ::lm::mat4& A,
            const ::lm::mat4& B) noexcept {
            ::lm::mat4 R{};

            for (int c = 0; c < 4; ++c) {
                float32x4_t r =
                    vmulq_n_f32(vld1q_f32(A[0].data()), B[c][0]);

                r = vmlaq_n_f32(r, vld1q_f32(A[1].data()), B[c][1]);
                r = vmlaq_n_f32(r, vld1q_f32(A[2].data()), B[c][2]);
                r = vmlaq_n_f32(r, vld1q_f32(A[3].data()), B[c][3]);

                vst1q_f32(&R[c][0], r);
            }
            return R;
        }
#endif

    } // namespace detail

    // ============================================================
    // Identity
    // ============================================================

    template<typename T, std::size_t N>
    LMATH_OUT mat<T,N,N> mat_identity() noexcept {
        mat<T,N,N> M{};
        for (int i=0; i<N; ++i)
            for (int j=0; j<N; ++j)
                M[i][j] = T(i==j);
        return M;
    }

    // ============================================================
    // Basic ops
    // ============================================================

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> mat_add(const mat<T,C,R>& A,
                                 const mat<T,C,R>& B) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] + B[c];
        return M;
    }

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> mat_sub(const mat<T,C,R>& A,
                                 const mat<T,C,R>& B) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] - B[c];
        return M;
    }

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> mat_scale(const mat<T,C,R>& A,
                                                   T S) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] * S;
        return M;
    }

    // ============================================================
    // Multiplication
    // ============================================================

    template<typename T, std::size_t N>
    LMATH_OUT mat<T,N,N> mat_mul(const mat<T,N,N>& A,
                                 const mat<T,N,N>& B) noexcept {
        mat<T,N,N> R{};
        for (int c=0; c<N; ++c)
            for (int r=0; r<N; ++r) {
                T sum{};  for (int k=0; k<N; ++k)  sum += A[k][r] * B[c][k];
                R[c][r] = sum;
            }
        return R;
    }

    // SIMD-specialization
    inline mat4 mat4_mul(const mat4& A,
                         const mat4& B) noexcept {
        switch (simd::max_level()) {
#if defined(__ARM_NEON)
        case simd::level::neon:
            return detail::mat4_mul_neon(A,B);
#endif
#if defined(__SSE2__)
        case simd::level::sse2:
        case simd::level::avx:
        case simd::level::avx2:
            return detail::mat4_mul_sse2(A,B);
#endif
        default:
            return mat_mul<float, 4>(A,B); // generic fallback
        } // switch
    } // mat_mul

    template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> mat_mul_vec(const mat<T,N,N>& M,
                                     const vec<T,N>& V) noexcept {
        vec<T,N> R{};
        for (int r=0; r<N; ++r) {
            T s{};
            for (int c=0; c<N; ++c)
                s += M[c][r] * V[c];
            R[r] = s;
        }
        return R;
    }

    // SIMD-specialization
    inline vec4 mat4_mul_vec(const mat4& M,
                             const vec4& V) noexcept {
        switch (simd::max_level()) {
#if defined(__ARM_NEON)
        case simd::level::neon:
            return detail::mat4_mul_vec_neon(M,V);
#endif
#if defined(__SSE2__)
        case simd::level::sse2:
        case simd::level::avx:
        case simd::level::avx2:
            return detail::mat4_mul_vec_sse2(M,V);
#endif
        default:
            return mat_mul_vec<float, 4>(M,V);
        } // switch
    } // mat_mul_vec

    // ============================================================
    // Transpose
    // ============================================================

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,R,C> mat_transpose(const mat<T,C,R>& M) noexcept {
        mat<T,R,C> Rm{};
        for (int c=0; c<C; ++c)
            for (int r=0; r<R; ++r)
                Rm[r][c] = M[c][r];
        return Rm;
    }


    // ============================================================
    // mat2x3 transforms
    // ============================================================

    template<typename T>
    LMATH_OUT mat2x3_of<T> mat2x3_translate(T x, T y) noexcept {
        mat2x3_of<T> M{};
        M[0][0] = 1; M[1][1] = 1;
        M[1][2] = x;
        M[0][2] = y;
        return M;
    }

    template<typename T>
    LMATH_OUT mat2x3_of<T> mat2x3_rotate(T a) noexcept {
        T c = ::lm::cosf(a);
        T s = ::lm::sinf(a);
        return { {
            { c, s, 0 },
            {-s, c, 0 }
        } };
    }

    // ============================================================
    // mat3 transforms
    // ============================================================

    LMATH_OUT mat3 mat3_translate(float x, float y) noexcept {
        mat3 M = mat_identity<float,3>();
        M[2][0] = x;
        M[2][1] = y;
        return M;
    }

    LMATH_OUT mat3 mat3_scale(float x, float y) noexcept {
        mat3 M{};
        M[0][0] = x;
        M[1][1] = y;
        M[2][2] = 1.f;
        return M;
    }

    LMATH_OUT mat3 mat3_rotate(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        mat3 M{};
        M[0][0] =  c; M[1][0] = -s;
        M[0][1] =  s; M[1][1] =  c;
        M[2][2] = 1.f;
        return M;
    }

    // ============================================================
    // mat4 transforms
    // ============================================================

    LMATH_OUT mat4 mat4_translate(float x, float y, float z) noexcept {
        mat4 M = mat_identity<float,4>();
        M[3][0] = x;
        M[3][1] = y;
        M[3][2] = z;
        return M;
    }

    LMATH_OUT mat4 mat4_scale(float x, float y, float z) noexcept {
        mat4 M{};
        M[0][0] = x;
        M[1][1] = y;
        M[2][2] = z;
        M[3][3] = 1.f;
        return M;
    }

    LMATH_OUT mat4 mat4_rotate_x(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        mat4 M = mat_identity<float,4>();
        M[1][1] =  c; M[2][1] = -s;
        M[1][2] =  s; M[2][2] =  c;
        return M;
    }

    LMATH_OUT mat4 mat4_rotate_y(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        mat4 M = mat_identity<float,4>();
        M[0][0] =  c; M[2][0] =  s;
        M[0][2] = -s; M[2][2] =  c;
        return M;
    }

    LMATH_OUT mat4 mat4_rotate_z(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        mat4 M = mat_identity<float,4>();
        M[0][0] =  c; M[1][0] = -s;
        M[0][1] =  s; M[1][1] =  c;
        return M;
    }

    // ============================================================
    // Projection (OpenGL ES compatible)
    // ============================================================

    LMATH_OUT mat4 mat4_perspective(float fov_y,
                                    float aspect,
                                    float n,
                                    float f) noexcept {
        float a = 1.f / ::lm::tanf(fov_y * 0.5f);

        mat4 M{};
        M[0][0] = a / aspect;
        M[1][1] = a;
        M[2][2] = -(f+n)/(f-n);
        M[2][3] = -1.f;
        M[3][2] = -(2.f*f*n)/(f-n);
        return M;
    }

    LMATH_OUT mat4 mat4_ortho(float l, float r,
                              float b, float t,
                              float n, float f) noexcept {
        mat4 M{};
        M[0][0] =  2.f/(r-l);
        M[1][1] =  2.f/(t-b);
        M[2][2] = -2.f/(f-n);
        M[3][0] = -(r+l)/(r-l);
        M[3][1] = -(t+b)/(t-b);
        M[3][2] = -(f+n)/(f-n);
        M[3][3] =  1.f;
        return M;
    }


    // ============================================================
    // overloaded operators
    // ============================================================

    //  + -

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> operator+ (const mat<T,C,R>& A,
                                    const mat<T,C,R>& B) noexcept {
        return mat_add(A, B);
    }

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> operator- (const mat<T,C,R>& A,
                                    const mat<T,C,R>& B) noexcept {
        return mat_sub(A, B);
    }

    // scalar *

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,C,R> operator* (const mat<T,C,R>& A, T S) noexcept {
        return mat_scale(A,S);
    }

    template<typename T, std::size_t C1, std::size_t R1, std::size_t C2>
    LMATH_OUT mat<T,C2,R1> operator* (const mat<T,C1,R1>& A,
                                      const mat<T,C2,C1>& B) noexcept {
        return mat_mul(A,B);
    }

    template<typename T, std::size_t N>
    LMATH_CONSTEXPR vec<T,N> operator* (const mat<T,N,N>& M,
                                        const vec<T,N>&   V) noexcept {
        return mat_mul_vec(M,V);
    }

    template<typename T, std::size_t N>
    LMATH_CONSTEXPR vec<T,N> operator* (const vec<T,N>&   V,
                                        const mat<T,N,N>& M) noexcept {
        vec<T,N> res{};
        for (int c=0; c<N; ++c) {
            T s{};
            for (int k=0; k<N; ++k)
                s += V[k] * M[c][k];
            res[c] = s;
        }
        return res;
    }

    template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T,C,R> operator* (T S, const mat<T,C,R>& A) noexcept {
        return mat_scale(A,S);
    }

    // Compound operators (+= -= *= /=)

    template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T,C,R> operator/ (const mat<T,C,R>& A, T s) noexcept {
        return mat_scale(A, T(1)/s);
    }
    
    template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T,C,R>& operator*= (mat<T,C,R>& A, T s) noexcept {
        for (int c=0; c<C; ++c)
            A[c] *= s;
        return A;
    }

    // comparisons

    template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR bool operator== (const mat<T,C,R>& a,
                                     const mat<T,C,R>& b) noexcept {
        for (int c=0; c<C; ++c)
            for (int r=0; r<R; ++r)
                if (a[c][r] != b[c][r])
                    return false;
        return true;
    }

    template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR bool operator!= (const mat<T,C,R>& a,
                                     const mat<T,C,R>& b) noexcept {
        return !(a==b);
    }

} // namespace lm
