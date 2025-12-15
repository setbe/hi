#pragma once
#include <assert.h>

#include "../io.hpp"
#include "types.hpp"
#include "i_window.hpp"
#include "native/gl_loader.hpp"

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   include <Windows.h>
#endif

namespace hi {
    namespace native {
        // -------------------- Native Opengl Context -------------------------
        struct Opengl {
        private:
        #if defined(__linux__)
        #elif defined(_WIN32)
            HGLRC _hglrc{ nullptr };
        #endif
        
        public:
            explicit inline Opengl() noexcept = default;
            ~Opengl() noexcept;
            // Non-copyable, non-movable
            Opengl(const Opengl&) = delete;
            Opengl& operator=(const Opengl&) = delete;
            Opengl(Opengl&&) = delete;
            Opengl& operator=(Opengl&&) = delete;
        
        public:
            inline void Render(IWindow&) noexcept;
            inline void SwapBuffers(const IWindow&) const noexcept;
            IO_NODISCARD AboutError CreateContext(IWindow&) noexcept;
        
        #if defined(__linux__)
        
        #elif defined(_WIN32)
            HGLRC getHglrc() const noexcept { return _hglrc; }
            void setHglrc(HGLRC new_hglrc) noexcept { _hglrc = new_hglrc; }
        #endif
        }; // struct OpenglContext
    } // namespace native
} // namespace hi

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   include "../platform/windows/opengl_impl.hpp"
#endif