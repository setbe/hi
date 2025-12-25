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




// --- Diagnostics helpers (portable-ish) ---

#if defined(_MSC_VER)
#  define IO_DIAG_PUSH()            __pragma(warning(push))
#  define IO_DIAG_POP()             __pragma(warning(pop))
#  define IO_DIAG_DISABLE_MSVC(x)   __pragma(warning(disable: x))
#else
#  define IO_DIAG_PUSH()
#  define IO_DIAG_POP()
#  define IO_DIAG_DISABLE_MSVC(x)
#endif

#if defined(__clang__)
#  define IO_DIAG_CLANG_PUSH()      _Pragma("clang diagnostic push")
#  define IO_DIAG_CLANG_POP()       _Pragma("clang diagnostic pop")
#  define IO_DIAG_CLANG_IGNORE(w)   _Pragma(w)
#else
#  define IO_DIAG_CLANG_PUSH()
#  define IO_DIAG_CLANG_POP()
#  define IO_DIAG_CLANG_IGNORE(w)
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define IO_DIAG_GCC_PUSH()        _Pragma("GCC diagnostic push")
#  define IO_DIAG_GCC_POP()         _Pragma("GCC diagnostic pop")
#  define IO_DIAG_GCC_IGNORE(w)     _Pragma(w)
#else
#  define IO_DIAG_GCC_PUSH()
#  define IO_DIAG_GCC_POP()
#  define IO_DIAG_GCC_IGNORE(w)
#endif

// Unified push/pop
#define IO_DIAG_ALL_PUSH()  do { IO_DIAG_PUSH(); IO_DIAG_CLANG_PUSH(); IO_DIAG_GCC_PUSH(); } while(0)
#define IO_DIAG_ALL_POP()   do { IO_DIAG_CLANG_POP(); IO_DIAG_GCC_POP(); IO_DIAG_POP(); } while(0)

// Disable "use after move" warnings:
// - MSVC: C26800
// - Clang/GCC: try ignore -Wuse-after-move (if supported), otherwise harmless.
#if defined(__clang__)
#  define IO_DIAG_DISABLE_USE_AFTER_MOVE() \
     do { \
       IO_DIAG_DISABLE_MSVC(26800); \
       IO_DIAG_CLANG_IGNORE("clang diagnostic ignored \"-Wuse-after-move\""); \
     } while(0)
#elif defined(__GNUC__) && !defined(__clang__)
#  define IO_DIAG_DISABLE_USE_AFTER_MOVE() \
     do { \
       IO_DIAG_DISABLE_MSVC(26800); \
       IO_DIAG_GCC_IGNORE("GCC diagnostic ignored \"-Wuse-after-move\""); \
     } while(0)
#else
#  define IO_DIAG_DISABLE_USE_AFTER_MOVE() \
     do { IO_DIAG_DISABLE_MSVC(26800); } while(0)
#endif