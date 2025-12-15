/*
* The MIT License (MIT)
*
* Copyright © 2025 setbe
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the “Software”), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
* OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include "native/types.hpp"
#include "native/socket.hpp"
#include "native/out.hpp"
#include "native/battery.hpp"
#include "native/syscalls.hpp"
#include "native/atomic.hpp"
#include "native/containers.hpp"
#include "native/ptr.hpp"

#pragma region micro shit
// I ******* hate Microsoft products
#if defined(IO_NOSTD) && !defined(IO_MICROSHIT_NOSTD)
#   define IO_MICROSHIT_NOSTD
extern "C" {
    extern int _fltused;

    // required by compiler / CRT references (x86 decoration: _atexit)
    static int __cdecl _atexit(void(__cdecl* func)(void)) {
        // Ignore register, or impl simple stack here.
        (void)func; return 0;
    }

    // also provide plain atexit and onexit variants just in case
    static int __cdecl atexit(   void(__cdecl* func)(void)) { return _atexit(func); }
    static inline int __cdecl _purecall(void) { return 0; }
} // extern "C"

static void(__cdecl* _onexit(void(__cdecl* func)(void)))(void) {
    // minimal stub: return the pointer (but do not register)
    (void)func; return nullptr;
}
#endif
#pragma endregion