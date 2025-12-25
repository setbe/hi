#include "io.hpp"
#include "native/syscalls.hpp"
#include "native/slot_alloc.hpp"
#include "native/window.hpp"

#pragma region micro shit
// ============================================================================
// I *** hate Microsoft products
#if defined(IO_NOSTD) && defined(IO_MICROSHIT_NOSTD)
namespace std {
    extern const nothrow_t nothrow;
    using size_t = io::usize;
}

extern "C" {
    int _fltused = 0; // floating point used symbol

    // security cookie stubs (if you disabled /GS, but linker may still expect these)
    // uintptr_t __security_cookie = 0x0000;
    // static void __cdecl __security_check_cookie(uintptr_t) { (void)/*cookie*/0; }

    // issue: linker couldn't find memcpy function
    void* __cdecl memcpy(void* dest, const void* src, std::size_t count) {
        unsigned char* d = static_cast<unsigned char*>(dest);
        const unsigned char* s = static_cast<const unsigned char*>(src);
        while (count--) *d++ = *s++;
        return dest;
    }
    // issue: linker couldn't find memset function
    void* __cdecl memset(void* dest, int ch, std::size_t count) {
        unsigned char* d = static_cast<unsigned char*>(dest);
        while (count--) *d++ = static_cast<unsigned char>(ch);
        return dest;
    }

} // extern "C"
#endif // defined IO_NOSTD && defined HI_MICROSHIT
#pragma endregion


// ============================================================================

namespace hi {
    namespace global {
        unsigned char key_array[static_cast<size_t>(Key::__LAST__)]{0};
    } // namespace global
} // namespace hi


#if defined(IO_NOSTD)
// ============================================================================
// -------- Single object --------
void* __CRTDECL operator new(std::size_t size)                                 { return io::native::allocate_block(size); }
void __CRTDECL operator delete(void* ptr) noexcept                             { io::native::deallocate_block(ptr); }
void __CRTDECL operator delete(void* ptr, std::size_t) noexcept                { io::native::deallocate_block(ptr); }

// ------------ Array ------------
void* __CRTDECL operator new[](std::size_t size)                               { return io::native::allocate_block(size); }
void __CRTDECL operator delete[](void* ptr) noexcept                           { io::native::deallocate_block(ptr); }
void __CRTDECL operator delete[](void* ptr, std::size_t) noexcept              { io::native::deallocate_block(ptr); }

// ---- Nothrow single object ----
void* __CRTDECL operator new(std::size_t size, const std::nothrow_t&) noexcept { return io::native::allocate_block(size); }
void __CRTDECL operator delete(void* ptr, const std::nothrow_t&) noexcept      { io::native::deallocate_block(ptr); }

// -------- Nothrow array --------
void* __CRTDECL operator new[](std::size_t size, const std::nothrow_t&) noexcept { return io::native::allocate_block(size); }
void __CRTDECL operator delete[](void* ptr, const std::nothrow_t&) noexcept    { io::native::deallocate_block(ptr); }
#endif // IO_NOSTD