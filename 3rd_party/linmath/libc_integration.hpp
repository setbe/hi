#pragma once
#include <stddef.h> // size_t
#include <stdint.h> // uint32_t

#include "detail/feature_detection.hpp"

namespace lm {
    LMATH_CONSTEXPR_VAR float PI = 3.14159265359f;
    LMATH_CONSTEXPR_VAR float PI_HALF = 1.57079632679f;
    LMATH_CONSTEXPR_VAR float PI_DOUBLE = 6.28318530718f;

    LMATH_OUT float radians(float degrees) noexcept {
        return degrees * PI / 180.f;
    } // radians

    // ------------------------ Functions decl --------------------------------
    LMATH_OUT float sinf(float X) noexcept;
    LMATH_OUT float cosf(float X) noexcept;
    LMATH_OUT float tanf(float X) noexcept;
    LMATH_OUT float sqrtf(float X) noexcept; // Only one iteration. ~0.175 ulp.
    LMATH_OUT float floorf(float X) noexcept;


    // ------------------------ Functions impl --------------------------------
    LMATH_OUT float sinf(float X) noexcept {
        bool flip = false;
        const float x2 = X*X;

        // X to [0, 2pi)
        while (X >= PI_DOUBLE) X -= PI_DOUBLE;
        while (X < 0.f)        X += PI_DOUBLE;

        // [-pi/2, pi/2]
        if (X > PI) {
            X -= PI;
            flip = true;
        }
        if (X > PI_HALF) X = PI-X;

        // [-pi/2, pi/2]
        float result = X * (1.f - x2/6.f + x2*x2/120.0f);
        return flip ? -result : result;
    } // sinf


    LMATH_OUT float cosf(float X) noexcept { return sinf(X + PI_HALF); }


    LMATH_OUT float tanf(float X) noexcept {
        const float x2 = X * X;
        // [-pi, pi)
        while (X >  PI) X -= PI_DOUBLE;
        while (X < -PI) X += PI_DOUBLE;

        // tanf(X) ~= X + X^3/3 + 2x^5/15 + 17x^7/315
        return X + X*x2 *  (  1.f/3.f + x2*(2.f/15.f + x2*(17.f/315.f))  );
    } // tanf


    LMATH_OUT float sqrtf(float X) noexcept {
        if (X <= 0.f) return 0.f;

        float x_half = 0.5f * X;
        union { float f; uint32_t i; } u = { X }; // reinterpret as int
        u.i = 0x5f3759df - (u.i >> 1); // magic number

        float y = *(float*)&u.i; // TODO: UB
        y = y * (1.5f - x_half*y*y); // 1st Newton-Raphson iteration
        return X*y;
    } // sqrtf


    LMATH_OUT float floorf(float X) noexcept {
        int i = (int)X;
        if (X < 0.0f && X != static_cast<float>(i)) --i;
        return static_cast<float>(i);
    } // floorf
} // namespace lm